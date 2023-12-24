/**
 * @file historicaldataservice.hpp
 * @brief Defines the data types and Service for processing and persisting historical data.
 *
 * This file includes the definition of the HistoricalDataService, along with the associated
 * connector and listener classes. It handles the processing and storage of historical data for
 * various financial services.
 *
 * @author Niccolo Fabbri
 */

#ifndef HISTORICAL_DATA_SERVICE_HPP
#define HISTORICAL_DATA_SERVICE_HPP

#include <string>
#include <vector>
#include "soa.hpp"
#include "utils/utils.hpp"

using namespace std;

// Enum for various service types
enum ServiceType { POSITION, RISK, EXECUTION, STREAMING, INQUIRY, DEFAULT };


template<typename T>
class HistoricalDataConnector;
template<typename T>
class HistoricalDataListener;

/**
 * @class HistoricalDataService
 * @brief Service for processing and persisting historical data to a persistent store.
 *
 * This service manages historical data for various financial services, including position, risk,
 * execution, streaming, and inquiry data. It supports data persistence and notification to listeners.
 *
 * @tparam T The data type to persist, representing different types of financial data.
 */
template<typename T>
class HistoricalDataService : Service<string,T>
{
public:
    // Constructors and destructor
    HistoricalDataService();
    HistoricalDataService(ServiceType _type);
    ~HistoricalDataService();

    // Service interface methods
    T& GetData(string key);
    void OnMessage(T& data);
    void AddListener(ServiceListener<T>* listener);
    const vector<ServiceListener<T>*>& GetListeners() const;
    HistoricalDataConnector<T>* GetConnector();
    ServiceListener<T>* GetListener();
    ServiceType GetServiceType() const;

    // Method to persist data
    void PersistData(string persistKey, T& data);

private:
    unordered_map<string, T> hd;           ///< Historical data storage
    vector<ServiceListener<T>*> listeners; ///< Listeners for historical data updates
    HistoricalDataConnector<T>* connector; ///< Connector for historical data
    ServiceListener<T>* hdListener;        ///< Listener for historical data events
    ServiceType type;                      ///< Type of service handled
};
// **********************************************************************************
//                  Implementation of HistoricalDataService...
// **********************************************************************************
template<typename T>
HistoricalDataService<T>::HistoricalDataService()
{
    hd = unordered_map<string, T>();
    listeners = vector<ServiceListener<T>*>();
    connector = new HistoricalDataConnector<T>(this);
    hdListener = new HistoricalDataListener<T>(this);
    type = DEFAULT;
}

template<typename T>
HistoricalDataService<T>::HistoricalDataService(ServiceType _type)
{
    hd = unordered_map<string, T>();
    listeners = vector<ServiceListener<T>*>();
    connector = new HistoricalDataConnector<T>(this);
    hdListener = new HistoricalDataListener<T>(this);
    type = _type;
}
template<typename T>
HistoricalDataService<T>::~HistoricalDataService() {}


template<typename T>
void HistoricalDataService<T>::AddListener(ServiceListener<T>* listener)
{
    listeners.push_back(listener);
}

template<typename T>
const vector<ServiceListener<T>*>& HistoricalDataService<T>::GetListeners() const
{
    return listeners;
}

template<typename T>
HistoricalDataConnector<T>* HistoricalDataService<T>::GetConnector()
{
    return connector;
}

template<typename T>
ServiceListener<T>* HistoricalDataService<T>::GetListener()
{
    return hdListener;
}

template<typename T>
ServiceType HistoricalDataService<T>::GetServiceType() const
{
    return type;
}

template<typename T>
T& HistoricalDataService<T>::GetData(string key){
    auto it = hd.find(key);
    if (it != hd.end()) {
        return it->second;
    } else {
        // Handle the case where the key is not found. For example:
        throw std::runtime_error("Historical data not found for key: " + key);
    }
}

template<typename T>
void HistoricalDataService<T>::OnMessage(T& data){
    std::string productId = data.GetProduct().GetProductId();

    auto it = hd.find(productId);
    if (it != hd.end()) {
        // Update the existing position
        it->second = data;
    } else {
        // Insert the new position if it does not exist
        hd.insert({productId, data});
    }
}
template<typename T>
void HistoricalDataService<T>::PersistData(string persistKey, T& data)
{
    OnMessage(data);
    connector->Publish(data);
}


/**
 * @class HistoricalDataConnector
 * @brief Connector for the HistoricalDataService to publish data.
 *
 * This connector is responsible for publishing historical data from the HistoricalDataService
 * to a persistent storage, such as a file. It handles data persistence based on the service type,
 * writing to specific files for each type of historical data.
 *
 * @tparam T The data type representing different types of financial data.
 */
template<typename T>
class HistoricalDataConnector : public Connector<T>
{

public:
    // Constructor and Destructor
    HistoricalDataConnector(HistoricalDataService<T>* service);
    ~HistoricalDataConnector();

    // Publish data to the Connector
    void Publish(T& data);

    // Subscribe data from the Connector (not implemented)
    void Subscribe(ifstream& data);

private:
    HistoricalDataService<T>* hist; ///< Reference to the associated HistoricalDataService
    std::unordered_map<ServiceType, std::string> filePathMap;
};
// **********************************************************************************
//                  Implementation of HistoricalDataConnector...
// **********************************************************************************

template<typename T>
HistoricalDataConnector<T>::HistoricalDataConnector(HistoricalDataService<T>* service)
{
    hist = service;
    filePathMap[POSITION] = "../data/out/positions.txt";
    filePathMap[RISK] = "../data/out/risk.txt";
    filePathMap[EXECUTION] = "../data/out/executions.txt";
    filePathMap[STREAMING] = "../data/out/streaming.txt";
    filePathMap[INQUIRY] = "../data/out/allinquiries.txt";
}

template<typename T>
HistoricalDataConnector<T>::~HistoricalDataConnector() {}


template<typename T>
void HistoricalDataConnector<T>::Publish(T& data)
{
    ServiceType _type = hist->GetServiceType();
    auto it = filePathMap.find(_type);
    if (it != filePathMap.end()) {
        std::string filePath = it->second;

        // File writing operations
        std::ofstream _file(filePath, std::ios::app);
        if (!_file.is_open()) {
            std::cerr << "Failed to open file: " << filePath << std::endl;
            return;
        }


        _file << CurrentDateTimeWithMillis() << ",";
        std::vector<std::string> _strings = data.HDFormat();
        for (const auto& s : _strings) {
            _file << s << ",";
        }
        _file << std::endl;
        _file.close();
    } else {
        std::cerr << "Unknown service type" << std::endl;
    }
}

template<typename T>
void HistoricalDataConnector<T>::Subscribe(ifstream& _data) {}


/**
 * @class HistoricalDataListener
 * @brief Listener for the HistoricalDataService to process and forward data.
 *
 * This listener reacts to new or updated historical data from the HistoricalDataService.
 * Upon receiving this data, it forwards it to the HistoricalDataConnector for persistence,
 * ensuring data is stored for future reference and analysis.
 *
 * @tparam T The data type representing different types of financial data.
 */
template<typename T>
class HistoricalDataListener : public ServiceListener<T>
{

public:
    // Constructor and Destructor
    HistoricalDataListener(HistoricalDataService<T>* service);
    ~HistoricalDataListener();

    // Listener callback to process an add event to the Service
    void ProcessAdd(T& data);

    // Listener callback to process a remove event to the Service (not implemented)
    void ProcessRemove(T& data);

    // Listener callback to process an update event to the Service (not implemented)
    void ProcessUpdate(T& data);

private:
    HistoricalDataService<T>* histd; ///< Reference to the HistoricalDataService
};
// **********************************************************************************
//                  Implementation of HistoricalDataConnector...
// **********************************************************************************
template<typename T>
HistoricalDataListener<T>::HistoricalDataListener(HistoricalDataService<T>* service)
{
    histd = service;
}

template<typename T>
HistoricalDataListener<T>::~HistoricalDataListener() {}

template<typename T>
void HistoricalDataListener<T>::ProcessAdd(T& data)
{
    string _persistKey = data.GetProduct().GetProductId();
    histd->PersistData(_persistKey, data);
}

template<typename T>
void HistoricalDataListener<T>::ProcessRemove(T& data) {}

template<typename T>
void HistoricalDataListener<T>::ProcessUpdate(T& data) {}

#endif
