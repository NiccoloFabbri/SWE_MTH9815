cmake_minimum_required(VERSION 3.26)
project(SWE_MTH9815)

set(CMAKE_CXX_STANDARD 20)


include_directories(/opt/homebrew/Cellar/boost/1.83.0/include/)

add_executable(bond
        tradingsystem.cpp
        pricingservice.hpp
        utils/utils.hpp
        tradebookingservice.hpp
        positionservice.hpp
        riskservice.hpp
        marketdataservice.hpp
        algoexecutionservice.hpp
        executionservice.hpp
        algostreamingservice.hpp
        streamingservice.hpp
        GUIService.hpp
        inquiryservice.hpp
        historicaldataservice.hpp
)