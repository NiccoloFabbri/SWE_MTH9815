/**
 * @file positionservice.hpp
 * @brief Defines the data types and Service for managing positions in financial securities.
 *
 * This file includes the definition of the Position class, which manages the position of a particular
 * financial product in various books, and the PositionService, which manages positions across multiple books.
 *
 * @author Niccolo Fabbri
 */
#ifndef POSITION_SERVICE_HPP
#define POSITION_SERVICE_HPP

#include <string>
#include <map>
#include "soa.hpp"
#include "tradebookingservice.hpp"
#include "utils/utils.hpp"
using namespace std;

/**
 * @class Position
 * @brief Represents a position held in a particular book for a financial product.
 *
 * This class manages the quantity of a financial product held across different trading books.
 * It provides functions to get the position for a specific book and the aggregate position across all books.
 *
 * @tparam T The type of the financial product.
 */
template<typename T>
class Position
{

public:

  // ctor for a position
  Position() = default;
  Position(const T& _product);

  // Get the product
  const T& GetProduct() const;

  // Get the position quantity
  long GetPosition(string &book);

  // Get the aggregate position
  long GetAggregatePosition() const;

    void AddPosition(string book, long quantity);
    const map<string, long>& GetPositions() const;

    vector<string> HDFormat() const;
private:
  T product;
  map<string,long> positions;

};
// **********************************************************************************
//                  Implementation of Position...
// **********************************************************************************
template<typename T>
vector<string> Position<T>::HDFormat() const {
    vector<string> formattedOutput;
    string productId = product.GetProductId();

    // Start with the product ID
    formattedOutput.push_back(productId);

    // Loop through positions and add them to the output
    for (const auto& positionPair : positions) {
        string book = positionPair.first;
        long quantity = positionPair.second;

        // Format book and quantity into strings and add to the vector
        formattedOutput.push_back(book);
        formattedOutput.push_back(std::to_string(quantity));
    }

    return formattedOutput;
}

template<typename T>
Position<T>::Position(const T& _product) :
  product(_product)
{
}

template<typename T>
const T& Position<T>::GetProduct() const
{
  return product;
}

template<typename T>
long Position<T>::GetPosition(string &book)
{
  return positions[book];
}

template<typename T>
long Position<T>::GetAggregatePosition() const {
    long aggregate = 0;
    for (const auto& pair : positions) {
        aggregate += pair.second;
    }
    return aggregate;
}

template<typename T>
void Position<T>::AddPosition(string book, long quantity) {
    // Check if the book already exists in the map
    auto it = positions.find(book);
    if (it != positions.end()) {
        // If the book exists, update its position
        it->second += quantity;
    } else {
        // If the book does not exist, create a new entry
        positions[book] = quantity;
    }
}

template<typename T>
const map<string, long>& Position<T>::GetPositions() const {
    return positions;
}





template<typename T>
class TradeBookingPosListener;
/**
 * @class PositionService
 * @brief Manages positions for financial products across multiple trading books.
 *
 * This service is responsible for maintaining and updating positions of various financial products
 * across different trading books. It listens for trade updates and modifies positions accordingly.
 *
 * @tparam T The type of financial product.
 */
template<typename T>
class PositionService : public Service<string,Position <T> >
{
public:
    // Constructors and destructor
    PositionService();
    ~PositionService();

    // Service interface methods
    Position<T>& GetData(string key);
    void OnMessage(Position<T>& data);
    void AddListener(ServiceListener<Position<T>>* listener);
    const vector<ServiceListener<Position<T>>*>& GetListeners() const;
    TradeBookingPosListener<T>* GetListener();

    // Additional service methods
    void AddTrade(const Trade<T> &trade);
    const unordered_map<string, Position<T>>& GetPositions() const;

private:
    unordered_map<string, Position<T>> positions;  ///< Positions keyed by product identifier
    vector<ServiceListener<Position<T>>*> listeners; ///< Listeners for position updates
    TradeBookingPosListener<T>* bookListener; ///< Listener for trade booking updates

    // Internal methods
    void UpdatePositionFromTrade(const Trade<T>& trade, Position<T>& position);
    void MergePositions(const Position<T>& fromPosition, Position<T>& toPosition);
};
// **********************************************************************************
//                  Implementation of PositionService...
// **********************************************************************************
template<typename T>
const unordered_map<string, Position<T>>& PositionService<T>::GetPositions() const {
    return positions;
}


template<typename T>
PositionService<T>::PositionService() {
    positions = unordered_map<string, Position<T>>();
    listeners = vector<ServiceListener<Position<T>>*>();
    bookListener = new TradeBookingPosListener<T>(this);
}

template<typename T>
PositionService<T>::~PositionService(){}

template<typename T>
void PositionService<T>::AddListener(ServiceListener<Position<T>>* listener)
{
    listeners.push_back(listener);
}

template<typename T>
TradeBookingPosListener<T>* PositionService<T>::GetListener(){
    return bookListener;
}

template<typename T>
Position<T>& PositionService<T>::GetData(std::string key){
    auto it = positions.find(key);
    if (it != positions.end()) {
        return it->second;
    } else {
        throw std::runtime_error("Position not found for key: " + key);
    }
}

template<typename T>
void PositionService<T>::OnMessage(Position<T> &data) {
    std::string productId = data.GetProduct().GetProductId();

    // Check if the position already exists in the map
    auto it = positions.find(productId);
    if (it != positions.end()) {
        // Update the existing position
        it->second = data;
    } else {
        // Insert the new position if it does not exist
        positions.insert({productId, data});
    }

    // Notify listeners
    for (auto& lstn : listeners) {
        lstn->ProcessAdd(data);
    }
}

template<typename T>
const std::vector<ServiceListener<Position<T>>*>& PositionService<T>::GetListeners() const {
    return listeners;
}

template<typename T>
void PositionService<T>::UpdatePositionFromTrade(const Trade<T>& trade, Position<T>& position) {
    string book = trade.GetBook();
    long quantity = trade.GetQuantity();
    Side side = trade.GetSide();

    switch (side) {
        case BUY:
            position.AddPosition(book, quantity);
            break;
        case SELL:
            position.AddPosition(book, -quantity);
            break;
    }
}

template<typename T>
void PositionService<T>::MergePositions(const Position<T>& fromPosition, Position<T>& toPosition) {
    for (const auto& p : fromPosition.GetPositions()) {
        string book = p.first;
        long quantity = p.second;
        toPosition.AddPosition(book, quantity);
    }
}

template<typename T>
void PositionService<T>::AddTrade(const Trade<T>& trade) {
    T product = trade.GetProduct();
    string productId = product.GetProductId();

    Position<T> newPosition(product);
    UpdatePositionFromTrade(trade, newPosition);

    auto it = positions.find(productId);
    if (it != positions.end()) {
        MergePositions(it->second, newPosition);
        positions[productId] = newPosition; // Update existing position
    } else {
        positions.insert({productId, newPosition}); // Insert new position
    }

    for (auto& lstn: listeners) {
        lstn->ProcessAdd(newPosition);
    }
}


/**
 * @class TradeBookingPosListener
 * @brief Listens for trade bookings and updates positions in the PositionService.
 *
 * This listener is responsible for reacting to trade updates. When a new trade is booked,
 * it updates the positions for the respective financial product in the PositionService.
 *
 * @tparam T The type of financial product.
 */
template<typename T>
class TradeBookingPosListener : public ServiceListener<Trade<T>> // will listen for a Trade and convert into position
{
public:
    // Constructor and destructor
    TradeBookingPosListener(PositionService<T>* service);
    ~TradeBookingPosListener();

    // Listener interface methods
    void ProcessAdd(Trade<T>& data);
    void ProcessRemove(Trade<T>& data);
    void ProcessUpdate(Trade<T>& data);

private:
    PositionService<T>* pos; ///< Reference to the PositionService
};
// **********************************************************************************
//                  Implementation of TradeBookingPosListener...
// **********************************************************************************
template<typename T>
TradeBookingPosListener<T>::TradeBookingPosListener(PositionService<T>* service)
{
    pos = service;
}

template<typename T>
TradeBookingPosListener<T>::~TradeBookingPosListener() {}

// ADD the process
template<typename T>
void TradeBookingPosListener<T>::ProcessAdd(Trade<T>& data)
{

    //printInYellow("Position Listener Triggered");

    pos->AddTrade(data);

    // DEBUG PRINTING
   /* const auto& positions = pos->GetPositions();
    for (const auto& pair : positions) {
        const auto& position = pair.second;
        cout << "Product: " << pair.first << endl;
        for (const auto& bookPos : position.GetPositions()) {
            cout << "  Book: " << bookPos.first << ", Quantity: " << bookPos.second << endl;
        }
        cout << "  Aggregate Position: " << position.GetAggregatePosition() << endl;
    }*/
}

template<typename T>
void TradeBookingPosListener<T>::ProcessRemove(Trade<T>& _data) {}

template<typename T>
void TradeBookingPosListener<T>::ProcessUpdate(Trade<T>& _data) {}


#endif
