/**
 * @file algoexecutionservice.hpp
 * @brief Defines the data types and Service for algorithmic executions in financial markets.
 *
 * This file includes the definition of AlgoExecution, which represents an algorithmic execution order,
 * and AlgoExecutionService, which manages these execution orders. It also includes the listener class
 * that interacts with market data to create execution orders.
 *
 * Author: Niccolo Fabbri
 */

#include <string>
#include <vector>
#include "soa.hpp"
#include "utils/utils.hpp"
#include "executionservice.hpp"


using namespace std;


/**
 * @class AlgoExecution
 * @brief Represents an algorithmic execution order.
 *
 * This class encapsulates an ExecutionOrder object, which includes all details of an execution order.
 * It is used by the AlgoExecutionService for managing algorithmic trades.
 *
 * @tparam T The type of the financial product.
 */
template<typename T>
class AlgoExecution{
private:
    ExecutionOrder<T>* executionOrder;
public:
    // Constructor and Destructor
    AlgoExecution(const T & _product, PricingSide type,
                  string _orderId, OrderType _orderType,
                  double _price, long _visibleQuantity,
                  long _hiddenQuantity, string _parentOrderId,
                  bool _isChildOrder);

    ~AlgoExecution();

    // Getter
    ExecutionOrder<T>* GetExecutionOrder();
};
// **********************************************************************************
//                  Implementation of AlgoExecution...
// **********************************************************************************
template<typename T>
AlgoExecution<T>::AlgoExecution(const T& _product, PricingSide _side,
                                string _orderId, OrderType _orderType,
                                double _price, long _visibleQuantity,
                                long _hiddenQuantity, string _parentOrderId,
                                bool _isChildOrder)
{
    executionOrder = new ExecutionOrder<T>(_product, _side, _orderId, _orderType, _price, _visibleQuantity, _hiddenQuantity, _parentOrderId, _isChildOrder);
}
template<typename T>
AlgoExecution<T>::~AlgoExecution() {}

template<typename T>
ExecutionOrder<T>* AlgoExecution<T>::GetExecutionOrder()
{
    return executionOrder;
}


// Forward Declaration
template<typename T>
class MDAlgoListener;

/**
 * @class AlgoExecutionService
 * @brief Service for managing algorithmic execution orders.
 *
 * This service is responsible for the creation and management of AlgoExecution orders,
 * which represent algorithmic execution strategies. It processes market data to generate
 * these orders and notifies listeners of new or updated executions.
 *
 * @tparam T The type of the financial product.
 */
template<typename T>
class AlgoExecutionService: public Service<string, AlgoExecution<T>>
{
private:
    unordered_map<string, AlgoExecution<T>> algoExe; ///< Storage for AlgoExecutions
    vector<ServiceListener<AlgoExecution<T>>*> listeners;///< Listeners for algo execution updates
    MDAlgoListener<T>* MDlistener; ///< Listener for market data
    double spread;///< Spread threshold for executions
    long side;///< Counter for managing execution orders


    AlgoExecution<T> CreateExecutionOrder(const OrderBook<T>& orderBook, PricingSide side, double price, long quantity);
    PricingSide DetermineOrderSide();
public:
    // Constructors and Destructor
    AlgoExecutionService();
    ~AlgoExecutionService();

    // Service interface methods
    AlgoExecution<T>& GetData(string key);
    void OnMessage(AlgoExecution<T>& data);
    void AddListener(ServiceListener<AlgoExecution<T>>* listener);
    const vector<ServiceListener<AlgoExecution<T>>*>& GetListeners() const;
    MDAlgoListener<T>* GetListener();

    // Method to execute algorithmic orders
    void AlgoExecuteOrder(OrderBook<T>& orderBook);
};
// **********************************************************************************
//                  Implementation of AlgoExecutionService...
// **********************************************************************************
template<typename T>
AlgoExecutionService<T>::AlgoExecutionService()
{
    algoExe = unordered_map<string, AlgoExecution<T>>();
    listeners = vector<ServiceListener<AlgoExecution<T>>*>();
    MDlistener = new MDAlgoListener<T>(this);
    spread = 1.0 / 128.0; // i need to cross the spread
    side = 0; //
}

template<typename T>
AlgoExecutionService<T>::~AlgoExecutionService() {}

template<typename T>
void AlgoExecutionService<T>::AddListener(ServiceListener<AlgoExecution<T>>* listener)
{
    listeners.push_back(listener);
}

template<typename T>
const vector<ServiceListener<AlgoExecution<T>>*>& AlgoExecutionService<T>::GetListeners() const
{
    return listeners;
}

template<typename T>
MDAlgoListener<T>* AlgoExecutionService<T>::GetListener()
{
    return MDlistener;
}

template<typename T>
AlgoExecution<T>& AlgoExecutionService<T>::GetData(std::string key){
    auto it = algoExe.find(key);
    if (it != algoExe.end()) {
        return it->second;
    } else {
        // Handle the case where the key is not found. For example:
        throw std::runtime_error("AlgoExe not found for key: " + key);
    }
}

template<typename T>
void AlgoExecutionService<T>::OnMessage(AlgoExecution<T> &data) {
    std::string productId = data.GetExecutionOrder()->GetProduct().GetProductId();

    auto it = algoExe.find(productId);
    if (it != algoExe.end()) {
        it->second = data;
    } else {
        algoExe.insert({productId, data});
    }

    // Notify listeners
    for (auto& lstn : listeners) {
        lstn->ProcessAdd(data);
    }
}

template<typename T>
AlgoExecution<T> AlgoExecutionService<T>::CreateExecutionOrder(const OrderBook<T>& orderBook, PricingSide side, double price, long quantity) {
    T product = orderBook.GetProduct();
    string orderId = GenerateRandomID(); // this function generates randoms ID for orders
    return AlgoExecution<T>(product, side, orderId, MARKET, price, quantity, 0, "", false);
}

template<typename T>
PricingSide AlgoExecutionService<T>::DetermineOrderSide() {
    return (side % 2 == 0) ? BID : OFFER;
}


template<typename T>
void AlgoExecutionService<T>::AlgoExecuteOrder(OrderBook<T>& orderBook) {
    T product = orderBook.GetProduct();
    string productId = product.GetProductId();

    // here I had to implement a get bid-ask method in the orderBook class
    BidOffer bidOffer = orderBook.GetBidAsk(); // get best bid and best ask

    Order bidOrder = bidOffer.GetBidOrder();
    Order offerOrder = bidOffer.GetOfferOrder();

    double bidPrice = bidOrder.GetPrice();
    double offerPrice = offerOrder.GetPrice();

    if (offerPrice - bidPrice <= spread) {
        PricingSide _side = DetermineOrderSide();
        double price = (_side == BID) ? bidPrice : offerPrice;
        long quantity = (_side == BID) ? bidOrder.GetQuantity() : offerOrder.GetQuantity();

        AlgoExecution<T> algoExecution = CreateExecutionOrder(orderBook, _side, price, quantity);
        OnMessage(algoExecution);

        side++;
    }
}



/**
 * @class MDAlgoListener
 * @brief Listener class that processes market data to trigger algorithmic executions.
 *
 * This listener responds to updates in market data (OrderBook) and creates AlgoExecution orders
 * based on this data, updating the AlgoExecutionService with these new or updated executions.
 *
 * @tparam T The type of the financial product.
 */

template<typename T>
class MDAlgoListener : public ServiceListener<OrderBook<T>>
{

private:
    AlgoExecutionService<T>* algo;///< Reference to AlgoExecutionService
public:

    // Constructor and Destructor
    MDAlgoListener(AlgoExecutionService<T>* service);
    ~MDAlgoListener();

    // Listener interface methods
    void ProcessAdd(OrderBook<T>& data);
    void ProcessRemove(OrderBook<T>& data);
    void ProcessUpdate(OrderBook<T>& data);

};
// **********************************************************************************
//                  Implementation of MDAlgoListener...
// **********************************************************************************
template<typename T>
MDAlgoListener<T>::MDAlgoListener(AlgoExecutionService<T>* service)
{
    algo = service;
}

template<typename T>
MDAlgoListener<T>::~MDAlgoListener() {}

template<typename T>
void MDAlgoListener<T>::ProcessAdd(OrderBook<T>& data)
{
    //printInYellow("Algo - MarketData listener triggered"); // DEBUG printing
    algo->AlgoExecuteOrder(data);
}

template<typename T>
void MDAlgoListener<T>::ProcessRemove(OrderBook<T>& data) {}

template<typename T>
void MDAlgoListener<T>::ProcessUpdate(OrderBook<T>& data) {}
