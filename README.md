# SWE_MTH9815

## Introduction
The Trading System is a  C++ program designed to emulate the operations of a real-world financial trading system. This simulation covers various aspects of a trading platform, including price feed handling, trade execution, risk management, market data processing, and customer inquiries.

## System Design
The `TradingSystem` class is at the heart of the simulation, integrating various services that represent different components of a financial trading system.

### Services
- **PricingService**: Manages and provides price data.
- **TradeBookingService**: Responsible for booking trades.
- **PositionService**: Tracks financial instrument positions.
- **RiskService**: Manages the risks of held positions.
- **MarketDataService**: Handles market data updates.
- **AlgoExecutionService**: Simulates algorithmic trading.
- **AlgoStreamingService**: Manages trading data streaming.
- **GUIService**: Simulates a graphical user interface.
- **ExecutionService**: Executes orders.
- **StreamingService**: Handles pricing information streaming.
- **InquiryService**: Manages customer trade inquiries.
- **HistoricalDataService**: Archives historical trading data.

## Class Design

The system is designed using template classes, allowing for flexibility in handling different types of financial products. Key classes include:

- `Price<T>`: Represents a price object with mid and bid/offer spread.
- `Trade<T>`: Encapsulates details of a trade, including product, side, and quantity.
- `Position<T>`: Tracks the position of a financial product in various books.
- `PV01<T>`: Represents the PV01 risk metric for a financial product.
- `ExecutionOrder<T>`: Models an execution order that can be placed on an exchange.
- `AlgoExecution<T>`: Represents an algorithmic execution order.
- `OrderBook<T>`: Manages an order book with bid and offer stacks for a product.
- `Inquiry<T>`: Models a customer inquiry for a financial product.

Services like `PricingService`, `TradeBookingService`, `RiskService`, etc., are built around these core data types, providing specialized functionalities.

`HistoricalData` in particular will write the outputs in the folder `data/out/{outputfile}.txt`. It relies on the `HDFormat` function to generate those files.

## Inter-Service Communication

The services communicate through listeners and connectors, which allow for real-time data processing and reaction to market events. For instance:

- `PricingService` feeds prices to `AlgoStreamingService` and `GUIService`.
- `MarketDataService` provides market data to `AlgoExecutionService` for creating execution orders.
- `ExecutionService` executes orders and notifies `TradeBookingService` and `HistoricalExecutionService`.
- `TradeBookingService` updates `PositionService`, which in turn updates `RiskService` and `HistoricalPositionService`.

## Simulation
### 1. Initialization and Linking
The system initializes all services and establishes connections using listeners to mimic the interconnected nature of real trading systems.

### 2. Simulated Data Processing
Data from text files representing prices, trades, market data, and inquiries are processed, emulating the flow of information in a trading platform.


### 3.Shutdown Process
Simulates end-of-day procedures, closing connections.

