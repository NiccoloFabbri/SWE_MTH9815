/**
 * @file inquiryservice.hpp
 * @brief Defines the data types and Service for handling customer inquiries.
 *
 * This file includes the definition of the Inquiry class, which models customer inquiries for financial products,
 * and the InquiryService, which manages these inquiries.
 *
 * @author Breman Thuraisingham
 */
#ifndef INQUIRY_SERVICE_HPP
#define INQUIRY_SERVICE_HPP

#include "soa.hpp"
#include "tradebookingservice.hpp"

// Various inqyury states
enum InquiryState { RECEIVED, QUOTED, DONE, REJECTED, CUSTOMER_REJECTED };

/**
 * Inquiry object modeling a customer inquiry from a client.
 * Type T is the product type.
 */
template<typename T>
class Inquiry
{

public:

  // ctor for an inquiry
  Inquiry(string _inquiryId, const T &_product, Side _side, long _quantity, double _price, InquiryState _state);

  // Get the inquiry ID
  const string& GetInquiryId() const;

  // Get the product
  const T& GetProduct() const;

  // Get the side on the inquiry
  Side GetSide() const;

  // Get the quantity that the client is inquiring for
  long GetQuantity() const;

  // Get the price that we have responded back with
  double GetPrice() const;

  // Get the current state on the inquiry
  InquiryState GetState() const;

  void SetState(InquiryState _state);

  void SetPrice(double _price);

  vector<string> HDFormat() const;

private:
  string inquiryId;
  T product;
  Side side;
  long quantity;
  double price;
  InquiryState state;

};

template<typename T>
vector<string> Inquiry<T>::HDFormat() const {
    vector<string> formattedOutput;
    string stateStr;
    switch (state) {
        case InquiryState::RECEIVED:
            stateStr = "RECEIVED";
            break;
        case InquiryState::QUOTED:
            stateStr = "QUOTED";
            break;
        case InquiryState::DONE:
            stateStr = "DONE";
            break;
        case InquiryState::REJECTED:
            stateStr = "REJECTED";
            break;
        case InquiryState::CUSTOMER_REJECTED:
            stateStr = "CUSTOMER_REJECTED";
            break;
        default:
            stateStr = "UNKNOWN";
            break;
    }
    // Add formatted elements to the vector
    formattedOutput.push_back(inquiryId);
    formattedOutput.push_back(product.GetProductId());
    formattedOutput.push_back(side == BUY ? "BUY" : "SELL");
    formattedOutput.push_back(std::to_string(quantity));
    formattedOutput.push_back(FormatPrice(price));
    formattedOutput.push_back(stateStr);

    return formattedOutput;
}
template<typename T>
Inquiry<T>::Inquiry(string _inquiryId, const T &_product, Side _side, long _quantity, double _price, InquiryState _state) :
        product(_product)
{
    inquiryId = _inquiryId;
    side = _side;
    quantity = _quantity;
    price = _price;
    state = _state;
}
template<typename T>
void Inquiry<T>::SetState(InquiryState _state) {
    state = _state;
}
template<typename T>
void Inquiry<T>::SetPrice(double _price) {
    price = _price;
}

template<typename T>
const string& Inquiry<T>::GetInquiryId() const
{
    return inquiryId;
}

template<typename T>
const T& Inquiry<T>::GetProduct() const
{
    return product;
}

template<typename T>
Side Inquiry<T>::GetSide() const
{
    return side;
}

template<typename T>
long Inquiry<T>::GetQuantity() const
{
    return quantity;
}

template<typename T>
double Inquiry<T>::GetPrice() const
{
    return price;
}

template<typename T>
InquiryState Inquiry<T>::GetState() const
{
    return state;
}


template<typename T>
class InquiryConnector;
template<typename T>
class InquiryListener;

/**
 * @class InquiryService
 * @brief Service for managing customer inquiries.
 *
 * This service handles customer inquiries, keyed on inquiry identifiers. It manages the lifecycle
 * of inquiries, including sending quotes and handling rejections.
 *
 * @tparam T The type of the financial product.
 */
template<typename T>
class InquiryService : public Service<string,Inquiry <T> >
{
public:
    // Constructors and destructor
    InquiryService();
    ~InquiryService();

    // Service interface methods
    Inquiry<T>& GetData(string key);
    void OnMessage(Inquiry<T>& data);
    void AddListener(ServiceListener<Inquiry<T>>* listener);
    const vector<ServiceListener<Inquiry<T>>*>& GetListeners() const;
    InquiryConnector<T>* GetConnector();

    // Inquiry management methods
    void SendQuote(const string &inquiryId, double price);
    void RejectInquiry(const string &inquiryId);

private:
    unordered_map<string, Inquiry<T>> inquiries;  ///< Inquiries keyed by inquiry ID
    vector<ServiceListener<Inquiry<T>>*> listeners; ///< Listeners for inquiry updates
    InquiryConnector<T>* connector;               ///< Connector for inquiry data
    InquiryListener<T>* inqlstn;                  ///< Listener for inquiry events
};
// **********************************************************************************
//                  Implementation of InquiryService...
// **********************************************************************************
template<typename T>
InquiryService<T>::InquiryService()
{
    inquiries = unordered_map<string, Inquiry<T>>();
    listeners = vector<ServiceListener<Inquiry<T>>*>();
    connector = new InquiryConnector<T>(this);
    inqlstn = new InquiryListener<T>(this);
    this->AddListener(inqlstn);

}
template<typename T>
InquiryService<T>::~InquiryService() {}

template<typename T>
void InquiryService<T>::AddListener(ServiceListener<Inquiry<T>>* listener)
{
    listeners.push_back(listener);
}
template<typename T>
const vector<ServiceListener<Inquiry<T>>*>& InquiryService<T>::GetListeners() const
{
    return listeners;
}

template<typename T>
InquiryConnector<T>* InquiryService<T>::GetConnector()
{
    return connector;
}


template<typename T>
Inquiry<T>& InquiryService<T>::GetData(string key){
    auto it = inquiries.find(key);
    if (it != inquiries.end()) {
        return it->second;
    } else {
        // Handle the case where the key is not found. For example:
        throw std::runtime_error("inquiry not found for key: " + key);
    }
}

template<typename T>
void InquiryService<T>::OnMessage(Inquiry<T>& data)
{
    string inquiryId = data.GetInquiryId();
    auto it = inquiries.find(inquiryId);
    if (it != inquiries.end()) {
        // Update the existing position
        it->second = data;
    } else {
        // Insert the new position if it does not exist
        inquiries.insert({inquiryId, data});
    }

    for (auto& lstn : listeners)
    {
        lstn->ProcessAdd(data);
    }
}

template<typename T>
void InquiryService<T>::SendQuote(const string& inquiryId, double price)
{
    Inquiry<T>& inq = GetData(inquiryId);
    InquiryState _state = inq.GetState();
    inq.SetPrice(price);
    connector->Publish(inq);
}

template<typename T>
void InquiryService<T>::RejectInquiry(const string& inquiryId)
{
    Inquiry<T>& _inquiry = GetData(inquiryId);
    _inquiry.SetState(REJECTED);
}

/**
 * @class InquiryConnector
 * @brief Connector for the InquiryService to publish and subscribe inquiry data.
 *
 * This connector manages the interaction between the InquiryService and external systems
 * or data sources. It handles both the subscription to external data sources for new inquiries
 * and the publication of inquiry updates to other parts of the system or external services.
 *
 * @tparam T The type of the financial product associated with the inquiry.
 */
template<typename T>
class InquiryConnector : public Connector<Inquiry<T>>
{
public:
    // Constructor and Destructor
    InquiryConnector(InquiryService<T>* service);
    ~InquiryConnector();

    // Publish inquiry data to the Connector
    void Publish(Inquiry<T>& data);

    // Subscribe to external data sources for inquiries
    void Subscribe(ifstream& data);

private:
    InquiryService<T>* inq; ///< Reference to the associated InquiryService
};
// **********************************************************************************
//                  Implementation of InquiryConnector...
// **********************************************************************************
template<typename T>
InquiryConnector<T>::InquiryConnector(InquiryService<T>* service)
{
    inq = service;
}
template<typename T>
InquiryConnector<T>::~InquiryConnector() {}


template<typename T>
void InquiryConnector<T>::Subscribe(std::ifstream& data) {
    string _line;
    while (getline(data, _line))
    {
        stringstream _lineStream(_line);
        string _cell;
        vector<string> _cells;
        while (getline(_lineStream, _cell, ','))
        {
            _cells.push_back(_cell);
        }

        string _inquiryId = _cells[0];
        string _productId = _cells[1];
        Side _side;
        if (_cells[2] == "BUY") _side = BUY;
        else if (_cells[2] == "SELL") _side = SELL;
        long _quantity = stol(_cells[3]);
        double _price = ConvertBondPrice(_cells[4]);
        InquiryState _state;
        if (_cells[5] == "RECEIVED\r") _state = InquiryState::RECEIVED;
        else if (_cells[5] == "QUOTED\r") _state = InquiryState::QUOTED;
        else if (_cells[5] == "DONE\r") _state = InquiryState::DONE;
        else if (_cells[5] == "REJECTED\r") _state = InquiryState::REJECTED;
        else if (_cells[5] == "CUSTOMER_REJECTED\r") _state = InquiryState::CUSTOMER_REJECTED;
        T _product = GetBond(_productId);
        Inquiry<T> _inquiry(_inquiryId, _product, _side, _quantity, _price, _state);
        inq->OnMessage(_inquiry);
    }
}

template<typename T>
void InquiryConnector<T>::Publish(Inquiry<T>& data)
{
    InquiryState _state = data.GetState();
    if (_state == RECEIVED)
    {
        data.SetState(QUOTED);
        inq->OnMessage(data);

        data.SetState(DONE);
        inq->OnMessage(data);
    }
}

/**
 * @class InquiryListener
 * @brief Listener for the InquiryService to process inquiry events.
 *
 * This listener reacts to new or updated inquiry data within the InquiryService. It can trigger
 * additional actions such as sending quotes or updating the state of an inquiry, based on the
 * current state and content of the inquiry data.
 *
 * @tparam T The type of the financial product associated with the inquiry.
 */
template<typename T>
class InquiryListener : public ServiceListener<Inquiry<T>>
{

public:
    // Constructor and Destructor
    InquiryListener(InquiryService<T>* service);
    ~InquiryListener();

    // Listener callback to process an add event to the Service
    void ProcessAdd(Inquiry<T>& data);

    // Listener callback to process a remove event to the Service (not implemented)
    void ProcessRemove(Inquiry<T>& data);

    // Listener callback to process an update event to the Service (not implemented)
    void ProcessUpdate(Inquiry<T>& data);

private:
    InquiryService<T>* inq; ///< Reference to the InquiryService
};
// **********************************************************************************
//                  Implementation of InquiryListener...
// **********************************************************************************
template<typename T>
InquiryListener<T>::InquiryListener(InquiryService<T>* service)
{
    inq = service;
}

template<typename T>
InquiryListener<T>::~InquiryListener() {}

template<typename T>
void InquiryListener<T>::ProcessAdd(Inquiry<T>& data)
{
    //printInYellow("Inquiry listener triggered");

    InquiryState state = data.GetState();
    if (state == RECEIVED){
        inq->SendQuote(data.GetInquiryId(), 100.0);
    }
}

template<typename T>
void InquiryListener<T>::ProcessRemove(Inquiry<T>& data) {}

template<typename T>
void InquiryListener<T>::ProcessUpdate(Inquiry<T>& data) {}




#endif

