/**
 * @file executionservice.hpp
 * @brief Defines the data types and Service for execution orders in financial markets.
 *
 * This file includes the definition of ExecutionOrder, which represents an order that can be placed on an exchange,
 * and the ExecutionService, which manages these orders.
 *
 * @author Niccolo Fabbri
 */
#ifndef EXECUTION_SERVICE_HPP
#define EXECUTION_SERVICE_HPP

#include <string>
#include "soa.hpp"
#include "marketdataservice.hpp"


enum OrderType { FOK, IOC, MARKET, LIMIT, STOP };

enum Market { BROKERTEC, ESPEED, CME };

/**
 * @class ExecutionOrder
 * @brief Represents an execution order that can be placed on an exchange.
 *
 * This class encapsulates all the details of an execution order, including product, side, order type,
 * price, quantities (visible and hidden), and parent order ID. It also indicates if it's a child order.
 *
 * @tparam T The type of the financial product.
 */
template<typename T>
class ExecutionOrder
{
public:
    // Constructor for an execution order
    ExecutionOrder(const T &_product, PricingSide _side, string _orderId, OrderType _orderType, double _price, double _visibleQuantity, double _hiddenQuantity, string _parentOrderId, bool _isChildOrder);

    // Getters
    const T& GetProduct() const;
    const string& GetOrderId() const;
    OrderType GetOrderType() const;
    double GetPrice() const;
    long GetVisibleQuantity() const;
    long GetHiddenQuantity() const;
    PricingSide GetPricingSide() const;
    const string& GetParentOrderId() const;
    bool IsChildOrder() const;

    // Formatted output for historical data
    vector<string> HDFormat() const;

private:
    T product;
    PricingSide side;
    string orderId;
    OrderType orderType;
    double price;
    double visibleQuantity;
    double hiddenQuantity;
    string parentOrderId;
    bool isChildOrder;
};
// **********************************************************************************
//                  Implementation of ExecutionOrder...
// **********************************************************************************
template<typename T>
vector<string> ExecutionOrder<T>::HDFormat() const {
    vector<string> formattedOutput;

    // Add formatted elements to the vector
    formattedOutput.push_back(product.GetProductId());
    formattedOutput.push_back(side == BID ? "BID" : "OFFER");
    formattedOutput.push_back(orderId);
    formattedOutput.push_back([this]{
        switch (orderType) {
            case FOK: return "FOK";
            case IOC: return "IOC";
            case MARKET: return "MARKET";
            case LIMIT: return "LIMIT";
            case STOP: return "STOP";
            default: return "UNKNOWN";
        }
    }());
    formattedOutput.push_back(FormatPrice(price));
    formattedOutput.push_back(std::to_string(visibleQuantity));
    formattedOutput.push_back(std::to_string(hiddenQuantity));
    formattedOutput.push_back(isChildOrder ? "YES" : "NO");

    return formattedOutput;
}

template<typename T>
ExecutionOrder<T>::ExecutionOrder(const T &_product, PricingSide _side, string _orderId, OrderType _orderType, double _price, double _visibleQuantity, double _hiddenQuantity, string _parentOrderId, bool _isChildOrder) :
        product(_product)
{
    side = _side;
    orderId = _orderId;
    orderType = _orderType;
    price = _price;
    visibleQuantity = _visibleQuantity;
    hiddenQuantity = _hiddenQuantity;
    parentOrderId = _parentOrderId;
    isChildOrder = _isChildOrder;
}

template<typename T>
const T& ExecutionOrder<T>::GetProduct() const
{
    return product;
}

template<typename T>
const string& ExecutionOrder<T>::GetOrderId() const
{
    return orderId;
}
template<typename T>
PricingSide ExecutionOrder<T>::GetPricingSide() const {
    return side;
}
template<typename T>
OrderType ExecutionOrder<T>::GetOrderType() const
{
    return orderType;
}

template<typename T>
double ExecutionOrder<T>::GetPrice() const
{
    return price;
}

template<typename T>
long ExecutionOrder<T>::GetVisibleQuantity() const
{
    return visibleQuantity;
}

template<typename T>
long ExecutionOrder<T>::GetHiddenQuantity() const
{
    return hiddenQuantity;
}

template<typename T>
const string& ExecutionOrder<T>::GetParentOrderId() const
{
    return parentOrderId;
}

template<typename T>
bool ExecutionOrder<T>::IsChildOrder() const
{
    return isChildOrder;
}


// Forward declarations
template<typename T>
class AlgoExecution;
template<typename T>
class AlgoExeExecutionListener;
/**
 * @class ExecutionService
 * @brief Service for executing orders on an exchange.
 *
 * This service manages execution orders for financial products. It processes orders, sends them to the market,
 * and notifies listeners of any updates.
 *
 * @tparam T The type of the financial product.
 */
template<typename T>
class ExecutionService : public Service<string,ExecutionOrder <T> >
{
public:
    // Constructors and destructor
    ExecutionService();
    ~ExecutionService();

    // Service interface methods
    ExecutionOrder<T>& GetData(string key);
    void OnMessage(ExecutionOrder<T>& data);
    void AddListener(ServiceListener<ExecutionOrder<T>>* listener);
    const vector<ServiceListener<ExecutionOrder<T>>*>& GetListeners() const;
    AlgoExeExecutionListener<T>* GetListener();

    // Execution method
    void ExecuteOrder(ExecutionOrder<T>& order, Market market);

private:
    unordered_map<string, ExecutionOrder<T>> exeOrd;  ///< Execution orders storage
    vector<ServiceListener<ExecutionOrder<T>>*> listeners; ///< Listeners for order updates
    AlgoExeExecutionListener<T>* algoExeListener;     ///< Listener for algorithmic execution events
};
// **********************************************************************************
//                  Implementation of ExecutionService...
// **********************************************************************************
template<typename T>
ExecutionService<T>::ExecutionService()
{
    exeOrd = unordered_map<string, ExecutionOrder<T>>();
    listeners = vector<ServiceListener<ExecutionOrder<T>>*>();
    algoExeListener = new AlgoExeExecutionListener<T>(this);
}

template<typename T>
ExecutionService<T>::~ExecutionService() {}

template<typename T>
void ExecutionService<T>::AddListener(ServiceListener<ExecutionOrder<T>>* listener)
{
    listeners.push_back(listener);
}

template<typename T>
const vector<ServiceListener<ExecutionOrder<T>>*>& ExecutionService<T>::GetListeners() const
{
    return listeners;
}

template<typename T>
AlgoExeExecutionListener<T>* ExecutionService<T>::GetListener()
{
    return algoExeListener;
}

template<typename T>
ExecutionOrder<T>& ExecutionService<T>::GetData(string key)
{
    auto it = exeOrd.find(key);
    if (it != exeOrd.end()) {
        return it->second;
    } else {
        throw std::runtime_error("Order not found for key: " + key);
    }
}


template<typename T>
void ExecutionService<T>::OnMessage(ExecutionOrder<T>& data)
{
    std::string productId = data.GetProduct().GetProductId();

    auto it = exeOrd.find(productId);
    if (it != exeOrd.end()) {
        it->second = data;
    } else {
        exeOrd.insert({productId, data});
    }

    // Notify listeners
    for (auto& lstn : listeners) {
        lstn->ProcessAdd(data);
    }
}


template<typename T>
void ExecutionService<T>::ExecuteOrder(ExecutionOrder<T>& order, Market market)
{
    OnMessage(order);
}


/**
 * @class AlgoExeExecutionListener
 * @brief Listener class that processes orders AlgoStreamingService.
 *
 * This listener responds to updates in the algo stream data and creates
 * execution orders from it.
 *
 * @tparam T The type of the financial product.
 */
template<typename T>
class AlgoExeExecutionListener : public ServiceListener<AlgoExecution<T>>
{

private:

    ExecutionService<T>* eOrder;

public:

    // Connector and Destructor
    AlgoExeExecutionListener(ExecutionService<T>* service);
    ~AlgoExeExecutionListener();

    // Listener interface methods
    void ProcessAdd(AlgoExecution<T>& data);
    void ProcessRemove(AlgoExecution<T>& data);
    void ProcessUpdate(AlgoExecution<T>& data);

};
// **********************************************************************************
//                  Implementation of AlgoExeExecutionListener...
// **********************************************************************************
template<typename T>
AlgoExeExecutionListener<T>::AlgoExeExecutionListener(ExecutionService<T>* service)
{
    eOrder = service;
}

template<typename T>
AlgoExeExecutionListener<T>::~AlgoExeExecutionListener() {}

template<typename T>
void AlgoExeExecutionListener<T>::ProcessAdd(AlgoExecution<T>& data)
{
    //printInYellow("Execution order - algo listener triggered");
    ExecutionOrder<T>* ord = data.GetExecutionOrder();
    //eOrder->OnMessage(*ord);
    eOrder->ExecuteOrder(*ord, CME);
}

template<typename T>
void AlgoExeExecutionListener<T>::ProcessRemove(AlgoExecution<T>& data) {}

template<typename T>
void AlgoExeExecutionListener<T>::ProcessUpdate(AlgoExecution<T>& data) {}




#endif
