/**
 * @file marketdataservice.hpp
 * @brief Defines the data types and service for order book market data.
 *
 * This file includes definitions for managing and distributing market data related to financial securities,
 * focusing on order books which consist of bid and offer orders.
 *
 * @author Niccolo Fabbri
 */
#ifndef MARKET_DATA_SERVICE_HPP
#define MARKET_DATA_SERVICE_HPP

#include <string>
#include <vector>
#include "soa.hpp"
#include "utils/utils.hpp"

using namespace std;

// Side for market data
enum PricingSide { BID, OFFER };

/**
 * A market data order with price, quantity, and side.
 */
class Order
{

public:

  // ctor for an order
  Order(double _price, long _quantity, PricingSide _side);

  // Get the price on the order
  double GetPrice() const;

  // Get the quantity on the order
  long GetQuantity() const;

  // Get the side on the order
  PricingSide GetSide() const;

private:
  double price;
  long quantity;
  PricingSide side;

};
// **********************************************************************************
//                  Implementation of Order...
// **********************************************************************************
Order::Order(double _price, long _quantity, PricingSide _side)
{
    price = _price;
    quantity = _quantity;
    side = _side;
}

double Order::GetPrice() const
{
    return price;
}

long Order::GetQuantity() const
{
    return quantity;
}

PricingSide Order::GetSide() const
{
    return side;
}

/**
 * Class representing a bid and offer order
 */
class BidOffer
{

public:

  // ctor for bid/offer
  BidOffer(const Order &_bidOrder, const Order &_offerOrder);

  // Get the bid order
  const Order& GetBidOrder() const;

  // Get the offer order
  const Order& GetOfferOrder() const;

private:
  Order bidOrder;
  Order offerOrder;

};
// **********************************************************************************
//                  Implementation of BidOffer...
// **********************************************************************************
BidOffer::BidOffer(const Order &_bidOrder, const Order &_offerOrder) :
        bidOrder(_bidOrder), offerOrder(_offerOrder)
{
}

const Order& BidOffer::GetBidOrder() const
{
    return bidOrder;
}

const Order& BidOffer::GetOfferOrder() const
{
    return offerOrder;
}

/**
 * Order book with a bid and offer stack.
 * Type T is the product type.
 */
template<typename T>
class OrderBook
{

public:

  // ctor for the order book
  OrderBook(const T &_product, const vector<Order> &_bidStack, const vector<Order> &_offerStack);

  // Get the product
  const T& GetProduct() const;

  // Get the bid stack
  const vector<Order>& GetBidStack() const;

  // Get the offer stack
  const vector<Order>& GetOfferStack() const;

  const BidOffer& GetBidAsk() const;

private:
  T product;
  vector<Order> bidStack;
  vector<Order> offerStack;

};
// **********************************************************************************
//                  Implementation of OrderBook...
// **********************************************************************************
template<typename T>
OrderBook<T>::OrderBook(const T &_product, const vector<Order> &_bidStack, const vector<Order> &_offerStack) :
        product(_product), bidStack(_bidStack), offerStack(_offerStack)
{
}

template<typename T>
const T& OrderBook<T>::GetProduct() const
{
    return product;
}

template<typename T>
const vector<Order>& OrderBook<T>::GetBidStack() const
{
    return bidStack;
}

template<typename T>
const vector<Order>& OrderBook<T>::GetOfferStack() const
{
    return offerStack;
}


template<typename T>
const BidOffer &OrderBook<T>::GetBidAsk() const {
    const auto& bids = GetBidStack();
    const auto& asks = GetOfferStack();

    const Order* bestBid = &bids.front();
    const Order* bestOffer = &asks.front();

    for (const auto& bid : bids) {
        if (bid.GetPrice() > bestBid->GetPrice()) {
            bestBid = &bid;
        }
    }

    for (const auto& offer : asks) {
        if (offer.GetPrice() < bestOffer->GetPrice()) {
            bestOffer = &offer;
        }
    }

    static BidOffer bestBidOffer(*bestBid, *bestOffer);
    return bestBidOffer;
}




// Forward Declaration
template<typename T>
class MarketDataConnector;
/**
 * @class MarketDataService
 * @brief Service to distribute market data, specifically order books.
 *
 * This template class provides functionality to manage and distribute market data for financial products.
 * It handles order books, updates them based on incoming data, and notifies listeners about these updates.
 *
 * @tparam T The type of the financial product.
 */
template<typename T>
class MarketDataService : public Service<string,OrderBook <T> >
{
public:
    // Constructors and destructor
    MarketDataService();
    ~MarketDataService();

    // Service interface methods
    OrderBook<T>& GetData(string key);
    void OnMessage(OrderBook<T>& data);
    void AddListener(ServiceListener<OrderBook<T>>* listener);
    const vector<ServiceListener<OrderBook<T>>*>& GetListeners() const;
    MarketDataConnector<T>* GetConnector();

    // Additional methods
    const BidOffer& GetBestBidOffer(const string &productId);
    const OrderBook<T>& AggregateDepth(const string &productId);
    int GetBookDepth() const;

private:
    unordered_map<string, OrderBook<T>> orderBooks;  ///< Order books keyed by product identifier
    vector<ServiceListener<OrderBook<T>>*> listeners; ///< Listeners for market data updates
    MarketDataConnector<T>* connector;               ///< Connector for market data
    int bookDepth;                                   ///< Depth of the order book

    // Helper methods
    vector<Order> AggregateOrders(const vector<Order>& orders, PricingSide type);
};
// **********************************************************************************
//                  Implementation of OrderBook...
// **********************************************************************************
template<typename T>
MarketDataService<T>::MarketDataService()
{
    orderBooks = unordered_map<string, OrderBook<T>>();
    listeners = vector<ServiceListener<OrderBook<T>>*>();
    connector = new MarketDataConnector<T>(this);
    bookDepth = 5;
}

template<typename T>
MarketDataService<T>::~MarketDataService() {}

template<typename T>
int MarketDataService<T>::GetBookDepth() const
{
    return bookDepth;
}

template<typename T>
void MarketDataService<T>::AddListener(ServiceListener<OrderBook<T>>* listener)
{
    listeners.push_back(listener);
}

template<typename T>
const vector<ServiceListener<OrderBook<T>>*>& MarketDataService<T>::GetListeners() const
{
    return listeners;
}

template<typename T>
MarketDataConnector<T>* MarketDataService<T>::GetConnector()
{
    return connector;
}


template<typename T>
OrderBook<T>& MarketDataService<T>::GetData(string key)
{
    auto it = orderBooks.find(key);
    if (it != orderBooks.end()) {
        return it->second;
    } else {
        throw std::runtime_error("OrderBook not found for key: " + key);
    }
}

template<typename T>
void MarketDataService<T>::OnMessage(OrderBook<T>& data)
{
    string productId = data.GetProduct().GetProductId();

    auto it = orderBooks.find(productId);
    if (it != orderBooks.end()) {
        // Update the existing order book
        it->second = data;
    } else {
        // Insert the new order book if it does not exist
        orderBooks.insert({productId, data});
    }

    // Notify listeners
    for (auto& lstn : listeners) {
        lstn->ProcessAdd(data);
    }
}

// #### GET BEST BID OFFER FUNCTION #####
template<typename T>
const BidOffer& MarketDataService<T>::GetBestBidOffer(const string& _productId)
{
    const auto& bids = orderBooks[_productId].GetBidStack();
    const auto& asks = orderBooks[_productId].GetOfferStack();

    // Check if the stacks are not empty
    if (bids.empty() || asks.empty()) {
        throw std::runtime_error("Bid or offer stack is empty for product: " + _productId);
    }

    // Initialize best bid and offer
    const Order* bestBid = &bids.front();
    const Order* bestOffer = &asks.front();

    // Find the highest bid
    for (const auto& bid : bids) {
        if (bid.GetPrice() > bestBid->GetPrice()) {
            bestBid = &bid;
        }
    }

    // Find the lowest offer
    for (const auto& offer : asks) {
        if (offer.GetPrice() < bestOffer->GetPrice()) {
            bestOffer = &offer;
        }
    }

    // Construct and return BidOffer
    static BidOffer bestBidOffer(*bestBid, *bestOffer);
    return bestBidOffer;
}

// ####### AGGREGATE DEPTH ######
template<typename T>
vector<Order> MarketDataService<T>::AggregateOrders(const vector<Order>& orders, PricingSide type) {
    unordered_map<double, long> priceToQuantity;
    for (const auto& order : orders) {
        priceToQuantity[order.GetPrice()] += order.GetQuantity();
    }

    vector<Order> aggregatedOrders;
    for (const auto& [price, quantity] : priceToQuantity) {
        aggregatedOrders.emplace_back(price, quantity, type);
    }
    return aggregatedOrders;
}

template<typename T>
const OrderBook<T>& MarketDataService<T>::AggregateDepth(const string& productId) {
    const auto& product = orderBooks[productId].GetProduct();
    auto bidStack = AggregateOrders(orderBooks[productId].GetBidStack(), BID);
    auto offerStack = AggregateOrders(orderBooks[productId].GetOfferStack(), OFFER);

    static OrderBook<T> aggregatedOrderBook(product, bidStack, offerStack);
    return aggregatedOrderBook;
}


/**
 * @class MarketDataConnector
 * @brief Connector for the MarketDataService to handle market data interaction.
 *
 * This connector is responsible for both subscribing to external sources of market data
 * (such as order books) and publishing market data updates. It serves as an interface
 * between the MarketDataService and external data sources or other services.
 *
 * @tparam T The type of the financial product associated with the market data.
 */
template<typename T>
class MarketDataConnector : public Connector<OrderBook<T>>
{
public:
    // Constructor and Destructor
    MarketDataConnector(MarketDataService<T>* _service);
    ~MarketDataConnector();

    // Publish market data updates to the Connector
    void Publish(OrderBook<T>& _data);

    // Subscribe to external sources for market data
    void Subscribe(ifstream& _data);

private:
    MarketDataService<T>* mkt; ///< Reference to the associated MarketDataService

    // Helper methods for parsing and creating orders
    std::tuple<std::string, double, long, PricingSide> ParseLine(const std::string& line);
    Order CreateOrder(const std::tuple<std::string, double, long, PricingSide>& orderData);
};
// **********************************************************************************
//                  Implementation of OrderBook...
// **********************************************************************************
template<typename T>
MarketDataConnector<T>::MarketDataConnector(MarketDataService<T>* service)
{
    mkt = service;
}

template<typename T>
MarketDataConnector<T>::~MarketDataConnector() {}

template<typename T>
void MarketDataConnector<T>::Publish(OrderBook<T>& _data) {}

template<typename T>
std::tuple<std::string, double, long, PricingSide> MarketDataConnector<T>::ParseLine(const std::string& line) {
    std::stringstream lineStream(line);
    std::string cell;
    std::vector<std::string> cells;

    while (std::getline(lineStream, cell, ',')) {
        cells.push_back(cell);
    }

    std::string productId = cells[0];
    double price = ConvertBondPrice(cells[1]);
    long quantity = std::stol(cells[2]);
    PricingSide side = (cells[3] == "BID" || cells[3] == "BID\r") ? BID : OFFER;

    return std::make_tuple(productId, price, quantity, side);
}

template<typename T>
Order MarketDataConnector<T>::CreateOrder(const std::tuple<std::string, double, long, PricingSide>& orderData) {
    double price;
    long quantity;
    PricingSide side;
    std::tie(std::ignore, price, quantity, side) = orderData;
    return Order(price, quantity, side);
}

template<typename T>
void MarketDataConnector<T>::Subscribe(std::ifstream& data) {
    int _bookDepth = mkt->GetBookDepth();
    int _thread = _bookDepth * 2;
    long _count = 0;
    std::vector<Order> _bidStack;
    std::vector<Order> _offerStack;
    std::string _line;

    while (std::getline(data, _line)) {
        auto orderData = ParseLine(_line);
        Order order = CreateOrder(orderData);
        PricingSide side = std::get<3>(orderData);
        if (side == BID)
            _bidStack.push_back(order);
        else
            _offerStack.push_back(order);

        _count++;
        if (_count % _thread == 0) {
            T _product = GetBond(std::get<0>(orderData));
            OrderBook<T> _orderBook(_product, _bidStack, _offerStack);
            mkt->OnMessage(_orderBook);
            _bidStack.clear();
            _offerStack.clear();
        }
    }
}


#endif
