/**
 * tradebookingservice.hpp
 * Defines the data types and Service for trade booking.
 *
 * @author Niccolo Fabbri
 */
#ifndef TRADE_BOOKING_SERVICE_HPP
#define TRADE_BOOKING_SERVICE_HPP

#include <string>
#include <vector>
#include <tuple>
#include <fstream>
#include <sstream>
#include "soa.hpp"
#include "utils/utils.hpp"
#include "executionservice.hpp"
// Trade sides
enum Side { BUY, SELL };

/**
 * Trade object with a price, side, and quantity on a particular book.
 * Type T is the product type.
 */
template<typename T>
class Trade
{

public:

  // ctor for a trade
  Trade(T &_product, string _tradeId, double _price, string _book, long _quantity, Side _side);

  // Get the product
  const T& GetProduct() const;

  // Get the trade ID
  const string& GetTradeId() const;

  // Get the mid price
  double GetPrice() const;

  // Get the book
  const string& GetBook() const;

  // Get the quantity
  long GetQuantity() const;

  // Get the side
  Side GetSide() const;

private:
  T product;
  string tradeId;
  double price;
  string book;
  long quantity;
  Side side;

};



template<typename T>
Trade<T>::Trade(T &_product, string _tradeId, double _price, string _book, long _quantity, Side _side) :
  product(_product)
{
  tradeId = _tradeId;
  price = _price;
  book = _book;
  quantity = _quantity;
  side = _side;
}

template<typename T>
const T& Trade<T>::GetProduct() const
{
  return product;
}

template<typename T>
const string& Trade<T>::GetTradeId() const
{
  return tradeId;
}

template<typename T>
double Trade<T>::GetPrice() const
{
  return price;
}

template<typename T>
const string& Trade<T>::GetBook() const
{
  return book;
}

template<typename T>
long Trade<T>::GetQuantity() const
{
  return quantity;
}

template<typename T>
Side Trade<T>::GetSide() const
{
  return side;
}

// fwd declaration for connector
template<typename T>
class TradeBookingConnector;
template<typename T>
class ExecutionBookingListener;

/**
 * @class TradeBookingService
 * @brief Service for booking trades for various financial products.
 *
 * This service manages the booking of trades for different types of financial products.
 * It maintains a record of all trades and notifies listeners about new or updated trades.
 * The service is keyed on the trade ID.
 *
 * @tparam T The type of the financial product.
 */
template<typename T>
class TradeBookingService : public Service<string,Trade <T> >
{
public:
    // Constructors and Destructor
    TradeBookingService();
    ~TradeBookingService();

    // Book a new trade
    void BookTrade(const Trade<T> &trade);

    // Service interface methods
    void OnMessage(Trade<T>& data);
    void AddListener(ServiceListener<Trade<T>>* listener);
    Trade<T>& GetData(std::string key);
    const std::vector<ServiceListener<Trade<T>>*>& GetListeners() const;

    // Get connectors to interact with external systems or data sources
    TradeBookingConnector<T>* GetConnector();
    ExecutionBookingListener<T>* GetListener();

private:
    std::unordered_map<std::string, Trade<T>> trades; // Storage for trades
    std::vector<ServiceListener<Trade<T>>*> listeners; // Listeners for trade events
    TradeBookingConnector<T>* connector; // Connector for trade data
    ExecutionBookingListener<T>* exeListener; // Listener for execution order events
};
// **********************************************************************************
//                  Implementation of TradeBookingService...
// **********************************************************************************
template<typename T>
TradeBookingService<T>::TradeBookingService() {
    trades = std::unordered_map<std::string, Trade<T>>();
    listeners = std::vector<ServiceListener<Trade<T>>*>();
    connector = new TradeBookingConnector<T>(this);
    exeListener = new ExecutionBookingListener<T>(this);
}

template<typename T>
TradeBookingService<T>::~TradeBookingService() {}

template<typename T>
TradeBookingConnector<T>* TradeBookingService<T>::GetConnector()
{
    return connector;
}

template<typename T>
void TradeBookingService<T>::OnMessage(Trade<T> &data) {
    // Save trade
    std::string tradeId = data.GetTradeId();
    auto it = trades.find(tradeId);
    if (it != trades.end()) {
        // Update the existing trade
        it->second = data;
    } else {
        // Insert the new trade
        trades.insert({tradeId, data});
    }


    for (auto listener: listeners){
        listener->ProcessAdd(data); // add the trade Object
    }
}

template<typename T>
void TradeBookingService<T>::AddListener(ServiceListener<Trade<T>>* listener)
{
    listeners.push_back(listener);
}

template<typename T>
ExecutionBookingListener<T>* TradeBookingService<T>::GetListener()
{
    return exeListener;
}

// Required Getters
template<typename T>
const std::vector<ServiceListener<Trade<T>>*>& TradeBookingService<T>::GetListeners() const {
    return listeners;
}
template<typename T>
Trade<T>& TradeBookingService<T>::GetData(std::string key){
    auto it = trades.find(key);
    if (it != trades.end()) {
        return it->second;
    } else {
        // Handle the case where the key is not found. For example:
        throw std::runtime_error("Trade not found for key: " + key);
    }
}

template<typename T>
void TradeBookingService<T>::BookTrade(const Trade<T> &trade)
{
    Trade<T> tradetobook = trade;
    OnMessage(tradetobook);
}




/**
 * @class TradeBookingConnector
 * @brief Connector for the TradeBookingService to handle trade data interaction.
 *
 * This connector is responsible for publishing trade data updates to external systems
 * and subscribing to trade data from external sources or other services.
 *
 * @tparam T The type of the financial product associated with the trades.
 */
template<typename T>
class TradeBookingConnector: public Connector<Trade<T>>{
public:
    // Constructor and Destructor
    TradeBookingConnector(TradeBookingService<T>* service);
    ~TradeBookingConnector();

    // Publish trade data updates to external systems or data sources
    void Publish(Trade<T> &data);

    // Subscribe to trade data from external systems or data sources
    void Subscribe(ifstream& data);

private:
    TradeBookingService<T>* _book; // Reference to the associated TradeBookingService

    // Helper methods for parsing and creating trades
    std::tuple<std::string, std::string, double, std::string, long, std::string> ParseLine(const std::string& line);
    Trade<T> CreateTrade(const std::tuple<std::string, std::string, double, std::string, long, std::string>& tradeData);
};

// **********************************************************************************
//                  Implementation of TradeBookingConnector...
// **********************************************************************************
template<typename T>
TradeBookingConnector<T>::TradeBookingConnector(TradeBookingService<T> *service) {
    //std::cout << "Initialized TradeBooking Connector"<< std::endl;
    _book = service;
}

// destructor
template<typename T>
TradeBookingConnector<T>::~TradeBookingConnector() {}

template<typename T>
void TradeBookingConnector<T>::Publish(Trade<T> &data) {}

template<typename T>
void TradeBookingConnector<T>::Subscribe(std::ifstream& data)
{
    std::string line;
    while (std::getline(data, line))
    {
        auto tradeData = ParseLine(line);
        Trade<T> trade = CreateTrade(tradeData);
/*        std::cout<< "Product: "<< trade.GetProduct();
        std::cout<< " Price: "<< trade.GetPrice();
        std::cout<< " Book: "<< trade.GetBook();
        std::cout<< " Quantity: "<< trade.GetQuantity();
        std::cout<< " Side: "<< trade.GetSide() << std::endl;*/
        _book->OnMessage(trade);
    }
}

template<typename T>
std::tuple<std::string, std::string, double, std::string, long, std::string> TradeBookingConnector<T>::ParseLine(const std::string& line)
{
    std::stringstream lineStream(line);
    std::string cell;
    std::vector<std::string> cells;

    while (std::getline(lineStream, cell, ','))
    {
        cells.push_back(cell);
    }

    std::string productId = cells[0];
    std::string tradeId = cells[1];
    double price = ConvertBondPrice(cells[2]);
    std::string book = cells[3];
    long quantity = std::stol(cells[4]);
    std::string side = cells[5];

    return std::make_tuple(productId, tradeId, price, book, quantity, side);
}

template<typename T>
Trade<T> TradeBookingConnector<T>::CreateTrade(const std::tuple<std::string, std::string, double, std::string, long, std::string>& tradeData)
{
    std::string productId, tradeId, book, sideStr;
    double price;
    long quantity;
    Side side;

    std::tie(productId, tradeId, price, book, quantity, sideStr) = tradeData;

    if (sideStr == "BUY") side = BUY;
    else if (sideStr == "SELL") side = SELL;

    T product = GetBond(productId);
    return Trade<T>(product, tradeId, price, book, quantity, side);
}



/**
 * @class ExecutionBookingListener
 * @brief Listener for execution orders to book trades in the TradeBookingService.
 *
 * This listener responds to execution order events, creating and booking corresponding trades
 * in the TradeBookingService. It processes add, remove, and update events for execution orders.
 *
 * @tparam T The type of the financial product.
 */
template<typename T>
class ExecutionBookingListener : public ServiceListener<ExecutionOrder<T>>
{
public:
    // Constructor and Destructor
    ExecutionBookingListener(TradeBookingService<T>* service);
    ~ExecutionBookingListener();

    // Listener callbacks for execution order events
    void ProcessAdd(ExecutionOrder<T>& data);
    void ProcessRemove(ExecutionOrder<T>& data);
    void ProcessUpdate(ExecutionOrder<T>& data);

private:
    TradeBookingService<T>* booking; // Reference to the TradeBookingService
    long count; // Counter to track the number of processed execution orders
    // Helper methods
    Trade<T> CreateTradeFromExecutionOrder(const ExecutionOrder<T>& order, const string& book, Side side);
    Side DetermineSideFromPricingSide(PricingSide pricingSide);
    string DetermineBookFromCount(long count);
};
// **********************************************************************************
//                  Implementation of ExecutionBookingListener...
// **********************************************************************************
template<typename T>
ExecutionBookingListener<T>::ExecutionBookingListener(TradeBookingService<T>* service)
{
    booking = service;
    count = 0;
}

template<typename T>
ExecutionBookingListener<T>::~ExecutionBookingListener() {}

template<typename T>
void ExecutionBookingListener<T>::ProcessAdd(ExecutionOrder<T>& data)
{
    count++;
    Side side = DetermineSideFromPricingSide(data.GetPricingSide());
    string book = DetermineBookFromCount(count);
    Trade<T> trade = CreateTradeFromExecutionOrder(data, book, side);
    booking->BookTrade(trade);
}

template<typename T>
Trade<T> ExecutionBookingListener<T>::CreateTradeFromExecutionOrder(const ExecutionOrder<T>& order, const string& book, Side side)
{
    T product = order.GetProduct();
    string tradeId = order.GetOrderId();
    double price = order.GetPrice();
    long quantity = order.GetVisibleQuantity() + order.GetHiddenQuantity();
    return Trade<T>(product, tradeId, price, book, quantity, side);
}

template<typename T>
Side ExecutionBookingListener<T>::DetermineSideFromPricingSide(PricingSide pricingSide)
{
    return (pricingSide == BID) ? SELL : BUY;
}

template<typename T>
string ExecutionBookingListener<T>::DetermineBookFromCount(long count)
{
    switch (count % 3)
    {
        case 0: return "TRSY1";
        case 1: return "TRSY2";
        case 2: return "TRSY3";
        default: return "TRSY1"; // Default case
    }
}

template<typename T>
void ExecutionBookingListener<T>::ProcessRemove(ExecutionOrder<T>& data) {}

template<typename T>
void ExecutionBookingListener<T>::ProcessUpdate(ExecutionOrder<T>& data) {}




#endif