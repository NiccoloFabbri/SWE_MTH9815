/**
 * @file algostreamingservice.hpp
 * @brief Defines the data types and Service for algorithmic streaming in financial markets.
 *
 * This file includes the definition of AlgoStream, which represents an algorithmic streaming data point,
 * and AlgoStreamingService, which manages these streams. Additionally, it includes the listener and connector
 * classes that interact with the streaming service.
 *
 * Author: Niccolo Fabbri
 */



#include <string>
#include <vector>
#include "soa.hpp"
#include "utils/utils.hpp"
#include "streamingservice.hpp"
#include "pricingservice.hpp"

using namespace std;

/**
 * @class AlgoStream
 * @brief Represents an algorithmic streaming data point.
 *
 * This class encapsulates a PriceStream object which represents the bid and offer prices for a financial product.
 *
 * @tparam T The type of the financial product.
 */
template<typename T>
class AlgoStream{
private:
    PriceStream<T>* priceStream;
public:
    // Constructor and Destructor
    AlgoStream(const T& _product, const PriceStreamOrder& _bidOrder, const PriceStreamOrder& _offerOrder);
    ~AlgoStream();

    // Getter for price stream
    PriceStream<T>* GetPriceStream() const;
};
// **********************************************************************************
//                  Implementation of AlgoStream...
// **********************************************************************************
template<typename T>
AlgoStream<T>::AlgoStream(const T& _product, const PriceStreamOrder& _bidOrder, const PriceStreamOrder& _offerOrder)
{
    priceStream = new PriceStream<T>(_product, _bidOrder, _offerOrder);
}

template<typename T>
AlgoStream<T>::~AlgoStream(){};

template<typename T>
PriceStream<T>* AlgoStream<T>::GetPriceStream() const
{
    return priceStream;
}


// Forward declaration
template<typename T>
class PricingASListener;

/**
 * @class AlgoStreamingService
 * @brief Service for managing algorithmic streams for financial products.
 *
 * This service is responsible for handling AlgoStreams, which include price streams
 * for financial products. It notifies listeners of new or updated algorithmic streams.
 *
 * @tparam T The type of the financial product.
 */
template<typename T>
class AlgoStreamingService : public Service<string, AlgoStream<T>>
{
public:
    // Constructors and destructor
    AlgoStreamingService();
    ~AlgoStreamingService();

    // Service interface methods
    AlgoStream<T>& GetData(string key);
    void OnMessage(AlgoStream<T>& data);
    void AddListener(ServiceListener<AlgoStream<T>>* listener);
    const vector<ServiceListener<AlgoStream<T>>*>& GetListeners() const;
    ServiceListener<Price<T>>* GetListener();

    // Additional methods
    void PublishPrice(Price<T>& price);

private:
    unordered_map<string, AlgoStream<T>> as; ///< Storage for AlgoStreams
    vector<ServiceListener<AlgoStream<T>>*> listeners; ///< Listeners for AlgoStream updates
    ServiceListener<Price<T>>* priceListener; ///< Listener for Price updates
    long count; ///< Counter for managing algo stream updates

};
// **********************************************************************************
//                  Implementation of AlgoStreamingService...
// **********************************************************************************
template<typename T>
AlgoStreamingService<T>::AlgoStreamingService()
{
    as = unordered_map<string, AlgoStream<T>>();
    listeners = vector<ServiceListener<AlgoStream<T>>*>();
    priceListener = new PricingASListener<T>(this);
    count = 0;
}

template<typename T>
AlgoStreamingService<T>::~AlgoStreamingService() {}

template<typename T>
void AlgoStreamingService<T>::AddListener(ServiceListener<AlgoStream<T>>* listener)
{
    listeners.push_back(listener);
}

template<typename T>
const vector<ServiceListener<AlgoStream<T>>*>& AlgoStreamingService<T>::GetListeners() const
{
    return listeners;
}

template<typename T>
ServiceListener<Price<T>>* AlgoStreamingService<T>::GetListener()
{
    return priceListener;
}

template<typename T>
AlgoStream<T>& AlgoStreamingService<T>::GetData(std::string key){
    auto it = as.find(key);
    if (it != as.end()) {
        return it->second;
    } else {
        // Handle the case where the key is not found. For example:
        throw std::runtime_error("Algostream not found for key: " + key);
    }
}

template<typename T>
void AlgoStreamingService<T>::OnMessage(AlgoStream<T> &data) {
    std::string productId = data.GetPriceStream()->GetProduct().GetProductId();

    auto it = as.find(productId);
    if (it != as.end()) {
        it->second = data;
    } else {
        as.insert({productId, data});
    }

    // Notify listeners
    for (auto& lstn : listeners) {
        lstn->ProcessAdd(data);
    }
}

template<typename T>
void AlgoStreamingService<T>::PublishPrice(Price<T> & price) {
    T product = price.GetProduct();

    double mid = price.GetMid();
    double bidOfferSpread = price.GetBidOfferSpread();
    // recreates bid and ask
    double bidPrice = mid - bidOfferSpread / 2.0;
    double offerPrice = mid + bidOfferSpread / 2.0;
    long visibleQuantity = (count % 2 + 1) * 10000000; // alternating
    long hiddenQuantity = visibleQuantity * 2; // double the visible

    count++;
    PriceStreamOrder bidOrder(bidPrice, visibleQuantity, hiddenQuantity, BID);
    PriceStreamOrder offerOrder(offerPrice, visibleQuantity, hiddenQuantity, OFFER);
    AlgoStream<T> algoStream(product, bidOrder, offerOrder);

    OnMessage(algoStream);
}

/**
 * @class PricingASListener
 * @brief Listener class that processes Price data and triggers AlgoStreamingService.
 *
 * This listener responds to updates in price data and creates AlgoStreams based on this data,
 * updating the AlgoStreamingService with these new or updated streams.
 *
 * @tparam T The type of the financial product.
 */
template<typename T>
class PricingASListener : public ServiceListener<Price<T>>
{

public:
    // Constructor and Destructor
    PricingASListener(AlgoStreamingService<T>* service);
    ~PricingASListener();

    // Listener interface methods
    void ProcessAdd(Price<T>& data);
    void ProcessRemove(Price<T>& data);
    void ProcessUpdate(Price<T>& data);

private:
    AlgoStreamingService<T>* algostrm; ///< Reference to AlgoStreamingService
};
// **********************************************************************************
//                  Implementation of PricingASListener...
// **********************************************************************************
template<typename T>
PricingASListener<T>::PricingASListener(AlgoStreamingService<T>* service)
{
    algostrm = service;
}

template<typename T>
PricingASListener<T>::~PricingASListener() {}

template<typename T>
void PricingASListener<T>::ProcessAdd(Price<T>& _data)
{
    //printInYellow("AlgoStream - Pricer listener triggered");
    algostrm->PublishPrice(_data);
}

template<typename T>
void PricingASListener<T>::ProcessRemove(Price<T>& _data) {}

template<typename T>
void PricingASListener<T>::ProcessUpdate(Price<T>& _data) {}
