/**
 * @file guiservice.hpp
 * @brief Defines the data types and Service for GUI (Graphical User Interface) interactions.
 *
 * This file includes the definition of the GUIService, along with the associated connector
 * and listener classes. It manages the data required for displaying prices and other information
 * on a GUI (txt file).
 *
 * @author Niccolo Fabbri
 */


#include <string>
#include <vector>
#include "soa.hpp"
#include "utils/utils.hpp"
#include "pricingservice.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>


using namespace std;

// Forward declarations
template<typename T>
class GUIConnector;
template<typename T>
class PricingGUIListener;

/**
 * @class GUIService
 * @brief Service for managing GUI-related data interactions.
 *
 * This service handles data required for the GUI, specifically price data. It manages updates
 * to this data and notifies listeners, as well as sends this data to a GUIConnector for display.
 *
 * @tparam T The type of the product for which the price is being managed.
 */
template<typename T>
class GUIService : Service<string, Price<T>>
{
public:
    // Constructors and destructor
    GUIService();
    ~GUIService();

    // Service interface methods
    Price<T>& GetData(string _key);
    void OnMessage(Price<T>& _data);
    void AddListener(ServiceListener<Price<T>>* _listener);
    const vector<ServiceListener<Price<T>>*>& GetListeners() const;
    GUIConnector<T>* GetConnector();
    ServiceListener<Price<T>>* GetListener();

    // Additional methods
    int GetThrottle() const;
    long GetMillisec() const;
    void SetMillisec(long _millisec);

private:
    unordered_map<string, Price<T>> guis;         ///< GUI data storage
    vector<ServiceListener<Price<T>>*> listeners; ///< Listeners for GUI data updates
    GUIConnector<T>* connector;                   ///< Connector for GUI data
    ServiceListener<Price<T>>* pricingListener;   ///< Listener for pricing data events
    int throttle;                                 ///< Throttling time for GUI updates
    long millisec;                                ///< Time in milliseconds for the last update
};
// **********************************************************************************
//                  Implementation of GUIService...
// **********************************************************************************
template<typename T>
GUIService<T>::GUIService()
{
    guis = unordered_map<string, Price<T>>();
    listeners = vector<ServiceListener<Price<T>>*>();
    connector = new GUIConnector<T>(this);
    pricingListener = new PricingGUIListener<T>(this);
    throttle = 300;
    millisec = 0;
}

template<typename T>
GUIService<T>::~GUIService() {}


template<typename T>
void GUIService<T>::AddListener(ServiceListener<Price<T>>* listener)
{
    listeners.push_back(listener);
}

template<typename T>
const vector<ServiceListener<Price<T>>*>& GUIService<T>::GetListeners() const
{
    return listeners;
}

template<typename T>
GUIConnector<T>* GUIService<T>::GetConnector()
{
    return connector;
}

template<typename T>
ServiceListener<Price<T>>* GUIService<T>::GetListener()
{
    return pricingListener;
}

template<typename T>
Price<T>& GUIService<T>::GetData(string key)
{
    auto it = guis.find(key);
    if (it != guis.end()) {
        return it->second;
    } else {
        throw std::runtime_error("Gui not found for key: " + key);
    }
}

template<typename T>
void GUIService<T>::OnMessage(Price<T> &data) {
    std::string productId = data.GetProduct().GetProductId();

    auto it = guis.find(productId);
    if (it != guis.end()) {

        it->second = data;
    } else {
        guis.insert({productId, data});
    }

    connector->Publish(data);
    // Notify listeners
    for (auto& lstn : listeners) {
        lstn->ProcessAdd(data);
    }
}


template<typename T>
int GUIService<T>::GetThrottle() const
{
    return throttle;
}

template<typename T>
long GUIService<T>::GetMillisec() const
{
    return millisec;
}

template<typename T>
void GUIService<T>::SetMillisec(long _millisec)
{
    millisec = _millisec;
}


/**
 * @class GUIConnector
 * @brief Connector for the GUIService to publish data.
 *
 * This connector is responsible for publishing price data from the GUIService to a GUI, typically a file.
 * It ensures data is published at a controlled rate, defined by a throttle mechanism.
 *
 * @tparam T The type of the product for which the price is being managed.
 */
template<typename T>
class GUIConnector : public Connector<Price<T>>
{
public:
    // Constructor and Destructor
    GUIConnector(GUIService<T>* service);
    ~GUIConnector();

    // Publish data to the Connector
    void Publish(Price<T>& data);

    // Subscribe data from the Connector (not implemented)
    void Subscribe(ifstream& data);

private:
    GUIService<T>* gui; ///< Reference to the associated GUIService
    long lastPublishTimeMillisec;
};
// **********************************************************************************
//                  Implementation of GUIConnector...
// **********************************************************************************
template<typename T>
GUIConnector<T>::GUIConnector(GUIService<T>* service)
{
    gui = service;
    lastPublishTimeMillisec = 0;
}

template<typename T>
GUIConnector<T>::~GUIConnector() {}

template<typename T>
void GUIConnector<T>::Subscribe(ifstream& _data) {}

template<typename T>
void GUIConnector<T>::Publish(Price<T>& data) {
    long throttle = gui->GetThrottle();
    long currentTime = GetCurrentTimeMillis();

    // Throttling check
    if (currentTime - lastPublishTimeMillisec >= throttle) {
        lastPublishTimeMillisec = currentTime; // Update the last publish time

        // Building the output string
        std::stringstream output;
        output << CurrentDateTimeWithMillis() << ",";
        for (const auto& s : data.GuiOut()) {
            output << s << ",";
        }
        output << "\n";

        // Writing to the file
        std::ofstream file("../data/gui.txt", std::ios::app);
        if (file.is_open()) {
            file << output.str();
            file.close();
        } else {
            std::cerr << "Error: Unable to open GUI output file." << std::endl;
        }
    }
}

/**
 * @class PricingGUIListener
 * @brief Listener for the PricingService to process and forward price data to GUIService.
 *
 * This listener reacts to new or updated price data from the PricingService. Upon receiving this data,
 * it forwards it to the GUIService for further processing and display in the GUI.
 *
 * @tparam T The type of the product for which the price is being managed.
 */
template<typename T>
class PricingGUIListener : public ServiceListener<Price<T>>
{
public:
    // Constructor and Destructor
    PricingGUIListener(GUIService<T>* service);
    ~PricingGUIListener();

    // Listener callback to process an add event to the Service
    void ProcessAdd(Price<T>& data);

    // Listener callback to process a remove event to the Service (not implemented)
    void ProcessRemove(Price<T>& data);

    // Listener callback to process an update event to the Service (not implemented)
    void ProcessUpdate(Price<T>& data);

private:
    GUIService<T>* gui; ///< Reference to the GUIService
};

// **********************************************************************************
//                  Implementation of PricingGUIListener...
// **********************************************************************************
template<typename T>
PricingGUIListener<T>::PricingGUIListener(GUIService<T>* service)
{
    gui = service;
}

template<typename T>
PricingGUIListener<T>::~PricingGUIListener() {}

template<typename T>
void PricingGUIListener<T>::ProcessAdd(Price<T>& data)
{
    gui->OnMessage(data);
}

template<typename T>
void PricingGUIListener<T>::ProcessRemove(Price<T>& data) {}

template<typename T>
void PricingGUIListener<T>::ProcessUpdate(Price<T>& data) {}
