//
// Created by Niccolò Fabbri on 24/12/23.
//
#include <iostream>
#include <fstream>
#include <thread>
// Include all necessary headers for your services
#include "pricingservice.hpp"
#include "tradebookingservice.hpp"
#include "positionservice.hpp"
#include "riskservice.hpp"
#include "marketdataservice.hpp"
#include "algoexecutionservice.hpp"
#include "algostreamingservice.hpp"
#include "GUIService.hpp"
#include "executionservice.hpp"
#include "streamingservice.hpp"
#include "inquiryservice.hpp"
#include "historicaldataservice.hpp"

using namespace std;

class TradingSystem {
private:
    // Declare all service objects
    PricingService<Bond> pricingService;
    TradeBookingService<Bond> tradeBookingService;
    PositionService<Bond> positionService;
    RiskService<Bond> riskService;
    MarketDataService<Bond> marketDataService;
    AlgoExecutionService<Bond> algoExeService;
    AlgoStreamingService<Bond> algoStreamingService;
    GUIService<Bond> guiService;
    ExecutionService<Bond> exeService;
    StreamingService<Bond> streamingService;
    InquiryService<Bond> inquiryService;
    HistoricalDataService<Position<Bond>> historicalPositionService;
    HistoricalDataService<PV01<Bond>> historicalRiskService;
    HistoricalDataService<ExecutionOrder<Bond>> historicalExecutionService;
    HistoricalDataService<PriceStream<Bond>> historicalStreamingService;
    HistoricalDataService<Inquiry<Bond>> historicalInquiryService;

public:
    TradingSystem() :
            historicalPositionService(POSITION),
            historicalRiskService(RISK),
            historicalExecutionService(EXECUTION),
            historicalStreamingService(STREAMING),
            historicalInquiryService(INQUIRY) {}

    void Initialize() {
        PrintInLightBlue("[Initialization] Setting up services...");
        this_thread::sleep_for(chrono::seconds(1));
        PrintInLightBlue("[Initialization] Services setup complete.");

        PrintInLightBlue("[Linking] Connecting services with listeners...");
        // Set up all listeners
        pricingService.AddListener(algoStreamingService.GetListener());
        pricingService.AddListener(guiService.GetListener());
        tradeBookingService.AddListener(positionService.GetListener());
        algoStreamingService.AddListener(streamingService.GetListener());
        streamingService.AddListener(historicalStreamingService.GetListener());
        marketDataService.AddListener(algoExeService.GetListener());
        algoExeService.AddListener(exeService.GetListener());
        exeService.AddListener(tradeBookingService.GetListener());
        exeService.AddListener(historicalExecutionService.GetListener());
        positionService.AddListener(riskService.GetListener());
        positionService.AddListener(historicalPositionService.GetListener());
        inquiryService.AddListener(historicalInquiryService.GetListener());
        riskService.AddListener(historicalRiskService.GetListener());
        this_thread::sleep_for(chrono::seconds(1));
        PrintInLightBlue("[Linking] Listeners connected successfully.");
    }

    void Run() {
        printInYellow("Starting Trading System...");
        this_thread::sleep_for(chrono::seconds(1));

        cout << "Receiving Prices..." << endl;
        std::ifstream priceData("../data/prices.txt");
        pricingService.GetConnector()->Subscribe(priceData);
        this_thread::sleep_for(chrono::milliseconds(500));

        cout << "Getting Trades Data..." << endl;
        std::ifstream tradeData("../data/trades.txt");
        tradeBookingService.GetConnector()->Subscribe(tradeData);
        this_thread::sleep_for(chrono::milliseconds(500));

        cout << "Loading Market Data..." << endl;
        std::ifstream marketData("../data/mktdata.txt");
        marketDataService.GetConnector()->Subscribe(marketData);
        this_thread::sleep_for(chrono::milliseconds(500));

        cout << "Loading inquiries..." << endl;
        ifstream inquiryData("../data/inquiries.txt");
        inquiryService.GetConnector()->Subscribe(inquiryData);
        this_thread::sleep_for(chrono::milliseconds(500));

    }

    ~TradingSystem() {
        printInYellow("The day is over, Shutting down Trading System...");
    }

};

int main() {
    TradingSystem tradingSystem;
    tradingSystem.Initialize();
    tradingSystem.Run();
    return 0;
}