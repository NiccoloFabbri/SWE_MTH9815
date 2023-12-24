/**
 * @file pricingservice.hpp
 * @brief Defines the data types and Service for internal prices.
 *
 * This file includes definitions for managing and distributing price data within
 * a financial trading system, focusing on mid prices and bid/offer spreads.
 *
 * @author Niccolo Fabbri
 */
#ifndef PRICING_SERVICE_HPP
#define PRICING_SERVICE_HPP

#include <string>
#include "soa.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include "utils/utils.hpp" // convert bond prices
/**
 * A price object consisting of mid and bid/offer spread.
 * Type T is the product type.
 */
template<typename T>
class Price
{

public:

  // ctor for a price
  Price(const T &_product, double _mid, double _bidOfferSpread);

  // Get the product
  const T& GetProduct() const;

  // Get the mid price
  double GetMid() const;

  // Get the bid/offer spread around the mid
  double GetBidOfferSpread() const;


  vector<string> GuiOut() const;

Price& operator=(const Price& other) {
    if (this != &other) {
        // Update other members, but cannot change 'product'
        this->mid = other.mid;
        this->bidOfferSpread = other.bidOfferSpread;
    }
    return *this;
}

private:
  const T& product;
  double mid;
  double bidOfferSpread;

};
// **********************************************************************************
//                  Implementation of Price...
// **********************************************************************************
template<typename T>
Price<T>::Price(const T &_product, double _mid, double _bidOfferSpread) :
        product(_product)
{
    mid = _mid;
    bidOfferSpread = _bidOfferSpread;
}

template<typename T>
const T& Price<T>::GetProduct() const
{
    return product;
}

template<typename T>
double Price<T>::GetMid() const
{
    return mid;
}

template<typename T>
double Price<T>::GetBidOfferSpread() const
{
    return bidOfferSpread;
}


template<typename T>
vector<string> Price<T>::GuiOut() const {
    // Assuming GetProductId, ConvertPrice functions are defined elsewhere
    string productID = product.GetProductId();
    string midPrice = FormatPrice(mid); // Format mid price to string
    string spread = FormatPrice(bidOfferSpread); // Format bid-offer spread to string

    vector<string> output;
    output.push_back(productID);
    output.push_back(midPrice);
    output.push_back(spread);
    return output;
}



// Forward declaration of PricingConnector
template<typename T>
class PricingConnector; // fwd declaration (writing in this file)

/**
 * @class PricingService
 * @brief Manages and distributes mid prices and bid/offers for financial products.
 *
 * This service is responsible for managing pricing information, specifically mid prices and bid/offer spreads,
 * and notifying listeners about price updates. It is keyed on the product identifier.
 *
 * @tparam T The type of the financial product.
 */
template<typename T>
class PricingService : public Service<std::string,Price <T> >
{
public:
    // Constructors and destructor
    PricingService();
    ~PricingService();

    // Service interface methods
    Price<T>& GetData(std::string key);
    void OnMessage(Price<T>& data);
    void AddListener(ServiceListener<Price<T>>* listener);
    const std::vector<ServiceListener<Price<T>>*>& GetListeners() const;
    PricingConnector<T>* GetConnector();

private:
    std::unordered_map<std::string, Price<T>> prices;  ///< Map of prices keyed by product identifier.
    std::vector<ServiceListener<Price<T>>*> listeners; ///< Listeners for price updates.
    PricingConnector<T>* connector;                    ///< Connector for reading price data.
};
// **********************************************************************************
//                  Implementation of PricingService...
// **********************************************************************************
template<typename T>
PricingService<T>::PricingService() {
    prices = std::unordered_map<std::string, Price<T>>();
    listeners = std::vector<ServiceListener<Price<T>>*>();
    connector = new PricingConnector<T>(this); // passing the context to the Pricing Connector
}

template<typename T>
PricingService<T>::~PricingService() {}

// Getter for the map
template<typename T>
Price<T>& PricingService<T>::GetData(std::string key) // return Price obj
{
    auto it = prices.find(key);
    if (it != prices.end()) {
        return it->second;
    } else {
        // Handle the case where the key is not found. For example:
        throw std::runtime_error("Price not found for key: " + key);
    }
    //return prices[key];
}

template<typename T>
PricingConnector<T>* PricingService<T>::GetConnector()
{
    return connector;
}

template<typename T>
void PricingService<T>::AddListener(ServiceListener<Price<T>>* listener)
{
    listeners.push_back(listener);
}

template<typename T>
void PricingService<T>::OnMessage(Price<T> &data) {
    std::string productId = data.GetProduct().GetProductId();
    auto it = prices.find(productId);
    if (it != prices.end()) {
        // Update existing entry. Since we cannot assign, we might need to replace the object.
        it->second = Price<T>(data.GetProduct(), data.GetMid(), data.GetBidOfferSpread());
    } else {
        // Insert new entry
        prices.insert({productId, data});
    }


    // notify listeners
    for (auto listener: listeners){ // update every listener
        listener->ProcessAdd(data);
    }
}

template<typename T>
const std::vector<ServiceListener<Price<T>>*>& PricingService<T>::GetListeners() const {
    return listeners;
}


/**
 * @class PricingConnector
 * @brief Connects and processes incoming price data for the PricingService.
 *
 * This class is responsible for reading and processing incoming price data, and updating
 * the PricingService with new price information. It handles the input data stream and parses
 * price information from it.
 *
 * @tparam T The type of the financial product.
 */
template<typename T>
class PricingConnector: public Connector<Price<T>>{ // derive connector
public:
    // Constructors and destructor
    PricingConnector(PricingService<T>* service);
    ~PricingConnector();

    // Overridden methods from Connector interface
    void Publish(Price<T> &data) override;
    void Subscribe(std::ifstream& data) override;

private:
    PricingService<T>* pricing;  ///< Reference to the associated PricingService.

    // Internal methods for processing data
    void ProcessLine(const string& _line);
    vector<string> SplitLine(std::stringstream& _lineStream);
    void ProcessCells(const vector<string>& _cells);
};
// **********************************************************************************
//                  Implementation of PricingConnector...
// **********************************************************************************
template<typename T>
PricingConnector<T>::PricingConnector(PricingService<T> *service) {
    pricing = service;
    //std::cout << "Initialized Pricing Connector"<< std::endl;
}


template<typename T>
void PricingConnector<T>::Subscribe(std::ifstream& data)
{
    //std::cout << "Subscribed called" << std::endl;
    std::string _line;
    while (std::getline(data, _line))
    {
        //std::cout << "streaming line..." << std::endl;
        ProcessLine(_line);
    }
    //std::cout << "finshed" << std::endl;
}
template<typename T>
void PricingConnector<T>::Publish(Price<T> &data) {}

// processing functions
template<typename T>
void PricingConnector<T>::ProcessLine(const std::string& _line)
{
    std::stringstream _lineStream(_line);
    std::vector<std::string> _cells = SplitLine(_lineStream);
    ProcessCells(_cells);
}

template<typename T>
std::vector<std::string> PricingConnector<T>::SplitLine(std::stringstream& _lineStream)
{
    std::vector<std::string> _cells;
    std::string _cell;
    while (std::getline(_lineStream, _cell, ','))
    {
        _cells.push_back(_cell);
    }
    return _cells;
}

template<typename T>
void PricingConnector<T>::ProcessCells(const std::vector<std::string>& _cells)
{
    std::string _productId = _cells[0];
    double bid = ConvertBondPrice(_cells[1]); // convert the price function
    double ask = ConvertBondPrice(_cells[2]);
    double mid = (bid + ask) / 2.0; // get mid
    double spread = ask - bid;

    T _product = GetBond(_productId); // get bond function
    Price<T> _price(_product, mid, spread);
    //std::cout << _product << " " << mid << " " << std::endl;
    pricing->OnMessage(_price);
}

#endif
