//
// Created by Niccolò Fabbri on 15/12/23.
//



#ifndef SWE_MTH9815_UTILS_HPP
#define SWE_MTH9815_UTILS_HPP


#include "../products.hpp" // needed
#include <sstream>
#include <string>
#include <chrono>
#include <random>
#include <boost/date_time/gregorian/gregorian.hpp>


double ConvertBondPrice(const std::string& price) {
    int whole, thirtySecond = 0, twoHundredFiftySixth = 0;
    char dash, plus;

    std::istringstream iss(price);
    iss >> whole >> dash >> thirtySecond;
    if (iss >> plus && plus == '+') {
        twoHundredFiftySixth = 4;
    }

    return whole + thirtySecond / 32.0 + twoHundredFiftySixth / 256.0;
}


std::string FormatPrice(double price) {
    int wholePart = static_cast<int>(price); // Extracts the whole number part
    double fractionalPart = (price - wholePart) * 32.0; // Converts the fractional part to 32nds

    int fractionalPart32 = static_cast<int>(fractionalPart); // Whole number of 32nds
    int fractionalPart64 = static_cast<int>((fractionalPart - fractionalPart32) * 2); // Checks for additional 1/64

    std::string formattedPrice = std::to_string(wholePart) + "-";
    formattedPrice += (fractionalPart32 < 10) ? "0" : ""; // Padding for single digit
    formattedPrice += std::to_string(fractionalPart32);

    if (fractionalPart64 == 1) {
        formattedPrice += "+"; // Add '+' if there's an additional 1/64
    }

    return formattedPrice;
}

Bond GetBond(string cusip)
{

    if (cusip == "91282CJL6") return Bond("91282CJL6", CUSIP, "US2Y", 0.04875, boost::gregorian::from_string("2025/11/30"));
    if (cusip == "91282CJK8") return Bond("91282CJK8", CUSIP, "US3Y", 0.04625, boost::gregorian::from_string("2026/11/15"));
    if (cusip == "91282CJN2") return Bond("91282CJN2", CUSIP, "US5Y", 0.04375, boost::gregorian::from_string("2028/11/30"));
    if (cusip == "91282CJM4") return Bond("91282CJM4", CUSIP, "US7Y", 0.04375, boost::gregorian::from_string("2030/11/30"));
    if (cusip == "91282CJJ1") return Bond("91282CJJ1", CUSIP, "US10Y", 0.045, boost::gregorian::from_string("2033/11/15"));
    if (cusip == "912810TW8") return Bond("912810TW8", CUSIP, "US20Y", 0.0475, boost::gregorian::from_string("2043/11/15"));
    if (cusip == "912810TV0") return Bond("912810TV0", CUSIP, "US30Y", 0.0475, boost::gregorian::from_string("2053/11/15"));
    return Bond();
}

void PrintInLightBlue(const std::string& message) {
    // ANSI escape code for light blue (cyan) text
    const std::string LIGHT_BLUE = "\033[96m";
    // ANSI escape code to reset color
    const std::string RESET = "\033[0m";

    std::cout << LIGHT_BLUE << message << RESET << std::endl;
}

void printInYellow(const std::string& message) {
    const std::string yellowCode = "33"; // ANSI color code for yellow
    std::cout << "\033[" << yellowCode << "m" << message << "\033[0m" << std::endl;
}

double calculatePV01(string cusip)
{
    double pv01 = 0;
    if (cusip == "91282CJL6") pv01 = 0.01;
    if (cusip == "91282CJK8") pv01 = 0.02;
    if (cusip == "91282CJN2") pv01 = 0.04;
    if (cusip == "91282CJM4") pv01 = 0.06;
    if (cusip == "91282CJJ1") pv01 = 0.08;
    if (cusip == "912810TW8") pv01 = 0.12;
    if (cusip == "912810TV0") pv01 = 0.20;
    return pv01;
}

std::string GenerateRandomID() {
    // Use current time as a base to ensure uniqueness over time
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto epoch = now_ms.time_since_epoch();
    auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
    long long timeNow = value.count();

    // Use a random number generator for additional randomness
    std::random_device rd; // Obtain a random number from hardware
    std::mt19937 eng(rd()); // Seed the generator
    std::uniform_int_distribution<> distr(1000, 9999); // Define the range

    // Combine the time and a random number into a string
    std::stringstream ss;
    ss << timeNow << "-" << distr(eng);

    return ss.str();
}


long GetCurrentTimeMillis() {
    using namespace std::chrono;

    // Get the current time point
    auto now = system_clock::now();

    // Convert the time point to the number of milliseconds since the Unix epoch
    auto epoch = now.time_since_epoch();
    auto millis = duration_cast<milliseconds>(epoch);

    return millis.count();
}

std::string CurrentDateTimeWithMillis() {
    using namespace std::chrono;

    // Get the current time point
    auto now = system_clock::now();

    // Convert to time_t for extracting date and time
    auto now_as_time_t = system_clock::to_time_t(now);

    // Convert to tm struct for formatting
    auto now_tm = *std::localtime(&now_as_time_t);

    // Extract milliseconds since the last second
    auto millis = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    // Format date, time, and milliseconds into a string
    std::stringstream ss;
    ss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << '.'
       << std::setfill('0') << std::setw(3) << millis.count();

    return ss.str();
}

#endif //SWE_MTH9815_UTILS_HPP
