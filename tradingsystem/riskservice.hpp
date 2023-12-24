/**
 * @file riskservice.hpp
 * @brief Defines data types and services for managing fixed income risk.
 *
 * This file includes the definition of PV01, BucketedSector, RiskService,
 * and PositionRiskListener classes, used for calculating and managing the risk
 * associated with fixed income securities.
 *
 * @author Niccolo Fabbri
 */
#ifndef RISK_SERVICE_HPP
#define RISK_SERVICE_HPP

#include "soa.hpp"
#include "positionservice.hpp"

/**
 * PV01 risk.
 * Type T is the product type.
 */
template<typename T>
class PV01
{

public:

  // ctor for a PV01 value
  PV01() = default;
  PV01(const T &_product, double _pv01, long _quantity);

  // Get the product on this PV01 value
  const T& GetProduct() const;

  // Get the PV01 value
  double GetPV01() const;

  // Get the quantity that this risk value is associated with
  long GetQuantity() const;

  vector<string> HDFormat() const;

private:
  T product;
  double pv01;
  long quantity;

};

template<typename T>
vector<string> PV01<T>::HDFormat() const {
    vector<string> formattedOutput;
    string productId = product.GetProductId();

    // Format pv01 and quantity into strings
    string formattedPV01 = to_string(pv01);
    string formattedQuantity = to_string(quantity);

    // Adding formatted elements to the vector
    formattedOutput.push_back(productId);
    formattedOutput.push_back(formattedPV01);
    formattedOutput.push_back(formattedQuantity);

    return formattedOutput;
}
template<typename T>
PV01<T>::PV01(const T &_product, double _pv01, long _quantity) :
        product(_product)
{
    pv01 = _pv01;
    quantity = _quantity;
}


template<typename T>
const T& PV01<T>::GetProduct() const
{
    return product;
}

template<typename T>
double PV01<T>::GetPV01() const
{
    return pv01;
}

template<typename T>
long PV01<T>::GetQuantity() const
{
    return quantity;
}

/**
 * @class BucketedSector
 * @brief Represents a bucketed sector for grouping and aggregating securities' risks.
 *
 * This class is used for aggregating risks of a group of securities into a single bucket.
 * It allows for the management of risk on a sector basis rather than individual securities.
 *
 * @tparam T The type of financial product in the bucket.
 */
template<typename T>
class BucketedSector
{

public:

  // ctor for a bucket sector
  BucketedSector(const vector<T> &_products, string _name);

  // Get the products associated with this bucket
  const vector<T>& GetProducts() const;

  // Get the name of the bucket
  const string& GetName() const;

private:
  vector<T> products;
  string name;

};


template<typename T>
BucketedSector<T>::BucketedSector(const vector<T>& _products, string _name) :
        products(_products)
{
    name = _name;
}

template<typename T>
const vector<T>& BucketedSector<T>::GetProducts() const
{
    return products;
}

template<typename T>
const string& BucketedSector<T>::GetName() const
{
    return name;
}

//Forward Declaration
template<typename T>
class PositionRiskListner;

/**
 * @class RiskService
 * @brief Service to manage and provide risk data for individual securities and risk sectors.
 *
 * This class provides functionality to calculate and provide risk metrics, specifically PV01,
 * for individual securities and aggregated risk for sectors. It handles positions and updates
 * the associated risk as needed.
 *
 * @tparam T The type of financial product.
 */
template<typename T>
class RiskService : public Service<string,PV01 <T> >
{
public:
    // Constructors
    RiskService();
    ~RiskService();

    // Service interface methods
    PV01<T>& GetData(string key);
    void OnMessage(PV01<T>& data);
    void AddListener(ServiceListener<PV01<T>>* _listener);
    const vector<ServiceListener<PV01<T>>*>& GetListeners() const;
    PositionRiskListner<T>* GetListener();

    // Risk management functions
    void AddPosition(Position<T> &position);
    PV01<BucketedSector<T>> GetBucketedRisk(const BucketedSector<T>& sector) const;

private:
    unordered_map<string, PV01<T>> pvs;              ///< Map of PV01 values keyed by product ID
    vector<ServiceListener<PV01<T>>*> listeners;     ///< Listeners for risk data changes
    PositionRiskListner<T>* posListener;             ///< Listener for position data changes
};
// **********************************************************************************
//                  Implementation of RiskService...
// **********************************************************************************
template<typename T>
RiskService<T>::RiskService()
{
    pvs = unordered_map<string, PV01<T>>();
    listeners = vector<ServiceListener<PV01<T>>*>();
    posListener = new PositionRiskListner<T>(this);
}

template<typename T>
RiskService<T>::~RiskService() {}

template<typename T>
void RiskService<T>::AddListener(ServiceListener<PV01<T>>* _listener)
{
    listeners.push_back(_listener);
}

template<typename T>
PositionRiskListner<T>* RiskService<T>::GetListener()
{
    return posListener;
}
template<typename T>
const std::vector<ServiceListener<PV01<T>>*>& RiskService<T>::GetListeners() const {
    return listeners;
}

template<typename T>
PV01<T>& RiskService<T>::GetData(string key)
{
    auto it = pvs.find(key);
    if (it != pvs.end()) {
        return it->second;
    } else {
        throw std::runtime_error("PV01 not found for key: " + key);
    }
}

template<typename T>
void RiskService<T>::OnMessage(PV01<T>& data)
{
    string productId = data.GetProduct().GetProductId();

    // Check if the product ID already exists in the map
    auto it = pvs.find(productId);
    if (it != pvs.end()) {
        // If the product ID exists, update the PV01 value
        it->second = data;
    } else {
        // If the product ID does not exist, insert the new PV01 value
        pvs.insert({productId, data});
    }

    // Notify listeners about the addition or update
    for (auto& lstn: listeners) {
        lstn->ProcessAdd(data);
    }
}

template<typename T>
void RiskService<T>::AddPosition(Position<T>& position)
{
    T product = position.GetProduct();
    string iD = product.GetProductId();
    double val = calculatePV01(iD);
    long qty = position.GetAggregatePosition();
    PV01<T> pv01(product, val, qty);

    auto it = pvs.find(iD);
    if (it != pvs.end()) {
        // Update existing entry
        it->second = pv01;
    } else {
        // Insert new entry
        pvs.insert({iD, pv01});
    }
    //this->GetBucketedRisk(BucketedSector<T>())
    // prendere product, prendere il sector
    for (auto& lstn: listeners)
    {
        lstn->ProcessAdd(pv01);
    }
}



template<typename T>
PV01<BucketedSector<T>> RiskService<T>::GetBucketedRisk(const BucketedSector<T>& sector) const
{
    double totalPV01 = 0;
    long totalQuantity = 0;

    // Aggregate PV01 and quantities for all products in the sector
    const auto& products = sector.GetProducts();
    for (const auto& product : products) {
        string productId = product.GetProductId();
        auto it = pvs.find(productId);
        if (it != pvs.end()) {
            totalPV01 += it->second.GetPV01() * it->second.GetQuantity();
            totalQuantity += it->second.GetQuantity();
        }
    }

    // Create and return a PV01 object for the entire sector
    // Note: You need to decide how to handle the product field for the sector PV01
    // For now, we are using a default-constructed BucketedSector<T> object
    BucketedSector<T> dummySector; // Assuming BucketedSector<T> has a default constructor
    return PV01<BucketedSector<T>>(dummySector, totalPV01, totalQuantity);
}




/**
 * @class PositionRiskListener
 * @brief Listener for position updates, responsible for updating risk data in RiskService.
 *
 * This listener responds to updates in position data and recalculates the associated risk
 * metrics. It is a crucial component in ensuring that risk calculations are always up-to-date
 * with the current position data.
 *
 * @tparam T The type of financial product.
 */
template<typename T>
class PositionRiskListner : public ServiceListener<Position<T>>
{

private:

    RiskService<T>* risk;

public:

    // Connector and Destructor
    PositionRiskListner(RiskService<T>* service);
    ~PositionRiskListner();

    // Listener callback to process an add event to the Service
    void ProcessAdd(Position<T>& _data);

    // Listener callback to process a remove event to the Service
    void ProcessRemove(Position<T>& _data);

    // Listener callback to process an update event to the Service
    void ProcessUpdate(Position<T>& _data);

};
// **********************************************************************************
//                  Implementation of PositionRiskListner...
// **********************************************************************************
template<typename T>
PositionRiskListner<T>::PositionRiskListner(RiskService<T>* service)
{
    risk = service;
}

template<typename T>
PositionRiskListner<T>::~PositionRiskListner() {}

template<typename T>
void PositionRiskListner<T>::ProcessAdd(Position<T>& _data)
{
    //printInYellow("Risk Listener triggered");
    risk->AddPosition(_data);
}

template<typename T>
void PositionRiskListner<T>::ProcessRemove(Position<T>& _data) {}

template<typename T>
void PositionRiskListner<T>::ProcessUpdate(Position<T>& _data) {}


#endif
