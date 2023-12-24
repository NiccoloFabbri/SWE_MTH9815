/**
 * streamingservice.hpp
 * Defines the data types and Service for price streams.
 * This file includes the definition of PriceStreamOrder, PriceStream, and StreamingService,
 * along with associated listener and utility functions. These are used to handle two-way price streams
 * in a market data environment.
 * @author Niccolo Fabbri
 */
#ifndef STREAMING_SERVICE_HPP
#define STREAMING_SERVICE_HPP

#include "soa.hpp"
#include "marketdataservice.hpp"

/**
 * @class PriceStreamOrder
 * @brief Represents an order in a price stream with price, quantity, and side (bid/offer).
 *
 * This class encapsulates the details of an individual order within a price stream,
 * including its price, visible and hidden quantities, and whether it's a bid or an offer.
 */
class PriceStreamOrder
{

public:

    /**
     * @brief Constructs a PriceStreamOrder with given price, visible and hidden quantities, and side.
     * @param _price Price of the order.
     * @param _visibleQuantity Visible quantity of the order.
     * @param _hiddenQuantity Hidden quantity of the order.
     * @param _side The side of the order (Bid or Offer).
     */
    PriceStreamOrder(double _price, long _visibleQuantity, long _hiddenQuantity, PricingSide _side);

    /**
  * @brief Gets the side of the order.
  * @return PricingSide The side of the order (Bid or Offer).
  */
    PricingSide GetSide() const;

    /**
     * @brief Gets the price of the order.
     * @return double The price of the order.
     */
  double GetPrice() const;

    /**
     * @brief Gets the visible quantity of the order.
     * @return long The visible quantity of the order.
     */
  long GetVisibleQuantity() const;

    /**
    * @brief Gets the hidden quantity of the order.
    * @return long The hidden quantity of the order.
    */
  long GetHiddenQuantity() const;

private:
  double price;
  long visibleQuantity;
  long hiddenQuantity;
  PricingSide side;

};
// **********************************************************************************
//                  Implementation of PriceStreamOrder...
// **********************************************************************************
PricingSide PriceStreamOrder::GetSide() const{
    return side;
}

PriceStreamOrder::PriceStreamOrder(double _price, long _visibleQuantity, long _hiddenQuantity, PricingSide _side)
{
    price = _price;
    visibleQuantity = _visibleQuantity;
    hiddenQuantity = _hiddenQuantity;
    side = _side;
}

double PriceStreamOrder::GetPrice() const
{
    return price;
}

long PriceStreamOrder::GetVisibleQuantity() const
{
    return visibleQuantity;
}

long PriceStreamOrder::GetHiddenQuantity() const
{
    return hiddenQuantity;
}


/**
 * @class PriceStream
 * @brief Represents a two-way market price stream for a financial product.
 *
 * This template class manages a two-way market price stream (bid and offer) for a given financial product.
 * It provides functionality to access the product, bid order, and offer order details.
 *
 * @tparam T The type of product associated with the price stream.
 */
template<typename T>
class PriceStream
{

public:

    /**
     * @brief Constructs a PriceStream with a specified product, bid order, and offer order.
     * @param _product The product associated with this price stream.
     * @param _bidOrder The bid order in the price stream.
     * @param _offerOrder The offer order in the price stream.
     */
  PriceStream(const T &_product, const PriceStreamOrder &_bidOrder, const PriceStreamOrder &_offerOrder);

    /**
     * @brief Gets the product associated with this price stream.
     * @return const T& A reference to the associated product.
     */
  const T& GetProduct() const;

    /**
     * @brief Gets the bid order of this price stream.
     * @return const PriceStreamOrder& A reference to the bid order.
     */
  const PriceStreamOrder& GetBidOrder() const;

    /**
   * @brief Gets the offer order of this price stream.
   * @return const PriceStreamOrder& A reference to the offer order.
   */
    const PriceStreamOrder& GetOfferOrder() const;

    /**
     * @brief Creates a formatted output for historical data service.
     * @return vector<string> The formatted data as a vector of strings.
     */
    vector<string> HDFormat()const;

private:
  T product;
  PriceStreamOrder bidOrder;
  PriceStreamOrder offerOrder;

};
// **********************************************************************************
//                  Implementation of PriceStream...
// **********************************************************************************
template<typename T>
vector<string> PriceStream<T>::HDFormat() const {
    vector<string> formattedOutput;
    // get the BidOrder infos
    formattedOutput.push_back(FormatPrice(bidOrder.GetPrice()));
    formattedOutput.push_back(std::to_string(bidOrder.GetVisibleQuantity()));
    formattedOutput.push_back(std::to_string(bidOrder.GetHiddenQuantity()));
    formattedOutput.push_back(bidOrder.GetSide() == BID ? "BID" : "OFFER");

    // get the OfferOrder infos
    formattedOutput.push_back(FormatPrice(offerOrder.GetPrice()));
    formattedOutput.push_back(std::to_string(offerOrder.GetVisibleQuantity()));
    formattedOutput.push_back(std::to_string(offerOrder.GetHiddenQuantity()));
    formattedOutput.push_back(offerOrder.GetSide() == BID ? "BID" : "OFFER");

    return formattedOutput;
}

template<typename T>
PriceStream<T>::PriceStream(const T &_product, const PriceStreamOrder &_bidOrder, const PriceStreamOrder &_offerOrder) :
        product(_product), bidOrder(_bidOrder), offerOrder(_offerOrder)
{
}

template<typename T>
const T& PriceStream<T>::GetProduct() const
{
    return product;
}

template<typename T>
const PriceStreamOrder& PriceStream<T>::GetBidOrder() const
{
    return bidOrder;
}

template<typename T>
const PriceStreamOrder& PriceStream<T>::GetOfferOrder() const
{
    return offerOrder;
}


// FORWARD DECLARATIONS FOR THE SERVICE
template<typename T>
class AlgoStream;
template<typename T>
class ASStreamingListener;


/**
 * @class StreamingService
 * @brief Service to publish two-way prices, keyed on product identifier.
 *
 * This template class provides functionality to manage and publish two-way price streams for financial products.
 * It maintains a collection of price streams and associated listeners.
 *
 * @tparam T The type of product associated with the price streams.
 */
template<typename T>
class StreamingService : public Service<string,PriceStream <T> >
{
private:
    // required attributes
    unordered_map<string, PriceStream<T>> pStreams;
    vector<ServiceListener<PriceStream<T>>*> listeners;
    ServiceListener<AlgoStream<T>>* asListener; // service listener

public:
    /**
   * @brief Constructs the StreamingService.
   */
    StreamingService();
    /**
   * @brief Destructor for StreamingService.
   */
    ~StreamingService();

    /**
  * @brief Retrieves a price stream for a given key.
  * @param key The key to identify the product.
  * @return PriceStream<T>& A reference to the associated price stream.
  * @throws std::runtime_error if the key is not found.
  */
    PriceStream<T>& GetData(string key);

    /**
      * @brief Handles incoming price stream data.
      * @param data The PriceStream object to process.
      */
    void OnMessage(PriceStream<T>& data);

    /**
   * @brief Adds a listener for price stream updates.
   * @param listener Pointer to the listener to add.
   */
    void AddListener(ServiceListener<PriceStream<T>>* listener);

    /**
   * @brief Gets the currently registered listeners.
   * @return const vector<ServiceListener<PriceStream<T>>*>& A const reference to the vector of listeners.
   */
    const vector<ServiceListener<PriceStream<T>>*>& GetListeners() const;

    /**
  * @brief Gets the listener for AlgoStream.
  * @return ServiceListener<AlgoStream<T>>* Pointer to the AlgoStream listener.
  */
    ServiceListener<AlgoStream<T>>* GetListener();


    /**
   * @brief Publishes a price stream to all registered listeners.
   * @param priceStream The PriceStream object to publish.
   */
    void PublishPrice(PriceStream<T>& priceStream);

};

// **********************************************************************************
//                  Implementation of StreamingService...
// **********************************************************************************
template<typename T>
StreamingService<T>::StreamingService()
{
    pStreams = unordered_map<string, PriceStream<T>>();
    listeners = vector<ServiceListener<PriceStream<T>>*>();
    asListener = new ASStreamingListener<T>(this);
}
template<typename T>
StreamingService<T>::~StreamingService() {}

template<typename T>
void StreamingService<T>::AddListener(ServiceListener<PriceStream<T>>* listener)
{
    listeners.push_back(listener);
}
template<typename T>
const vector<ServiceListener<PriceStream<T>>*>& StreamingService<T>::GetListeners() const
{
    return listeners;
}
template<typename T>
ServiceListener<AlgoStream<T>>* StreamingService<T>::GetListener()
{
    return asListener;
}

template<typename T>
PriceStream<T>& StreamingService<T>::GetData(string key){
    auto it = pStreams.find(key);
    if (it != pStreams.end()) {
        return it->second;
    } else {
        // Handle the case where the key is not found. For example:
        throw std::runtime_error("PriceStream not found for key: " + key);
    }
}
template<typename T>
void StreamingService<T>::OnMessage(PriceStream<T>& data){
    std::string productId = data.GetProduct().GetProductId();

    auto it = pStreams.find(productId);
    if (it != pStreams.end()) {
        // Update the existing position
        it->second = data;
    } else {
        // Insert the new position if it does not exist
        pStreams.insert({productId, data});
    }
    PublishPrice(data);

}

template<typename T>
void StreamingService<T>::PublishPrice(PriceStream<T>& _priceStream)
{

    for (auto& lstn : listeners) {
        lstn->ProcessAdd(_priceStream);
    }
}

/**
 * @class ASStreamingListener
 * @brief Listener for algorithmic streaming, handling events related to AlgoStream.
 *
 * This class is a specialized listener for handling algorithmic streaming events. It processes
 * add, remove, and update events for AlgoStream objects, and interacts with the StreamingService
 * to manage and update price streams accordingly.
 *
 * @tparam T The type of product associated with the AlgoStream.
 */
template<typename T>
class ASStreamingListener : public ServiceListener<AlgoStream<T>>
{

private:

    StreamingService<T>* stream;

public:

    /**
     * @brief Constructs an ASStreamingListener with a reference to a StreamingService.
     * @param service Pointer to the StreamingService that this listener will interact with.
     */

    ASStreamingListener(StreamingService<T>* service);
    /**
     * @brief Destructor for ASStreamingListener.
     */
    ~ASStreamingListener();

    /**
     * @brief Processes the addition of a new AlgoStream.
     *
     * This method is called when a new AlgoStream is added. It processes the AlgoStream and potentially
     * updates the associated StreamingService.
     *
     * @param data The AlgoStream object that has been added.
     */
    void ProcessAdd(AlgoStream<T>& data);

    /**
    * @brief Processes the removal of an AlgoStream.
    *
    * This method is called when an AlgoStream is removed. The implementation can handle any necessary cleanup or updates.
    *
    * @param data The AlgoStream object that has been removed.
    */
    void ProcessRemove(AlgoStream<T>& data);


    /**
     * @brief Processes updates to an existing AlgoStream.
     *
     * This method is invoked when an existing AlgoStream is updated. It can handle any logic necessary
     * to respond to changes in the AlgoStream.
     *
     * @param data The updated AlgoStream object.
     */
    void ProcessUpdate(AlgoStream<T>& data);

};
// **********************************************************************************
//                  Implementation of Listener...
// **********************************************************************************
template<typename T>
ASStreamingListener<T>::ASStreamingListener(StreamingService<T>* service)
{
    stream = service;
}

template<typename T>
ASStreamingListener<T>::~ASStreamingListener() {}

template<typename T>
void ASStreamingListener<T>::ProcessAdd(AlgoStream<T>& data)
{
    //printInYellow("Streaming from AS listener triggered");
    PriceStream<T>* _priceStream = data.GetPriceStream();
    stream->OnMessage(*_priceStream);
}

template<typename T>
void ASStreamingListener<T>::ProcessRemove(AlgoStream<T>& data) {}

template<typename T>
void ASStreamingListener<T>::ProcessUpdate(AlgoStream<T>& data) {}



#endif
