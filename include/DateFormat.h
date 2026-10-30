#ifndef DATE_FORMAT_H
#define DATE_FORMAT_H

#include <iomanip>
#include <sstream>
#include <string>
#include <ctime>

namespace DateFormat {

inline bool isDigits(const std::string& value)
{
    if (value.empty()) return false;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] < '0' || value[i] > '9') return false;
    }
    return true;
}

inline bool parseDate(const std::string& value, int& day, int& month, int& year)
{
    const std::size_t firstSeparator = value.find_first_of("/-");
    if (firstSeparator == std::string::npos) return false;
    const char separator = value[firstSeparator];
    const std::size_t secondSeparator = value.find(separator, firstSeparator + 1);
    if (secondSeparator == std::string::npos || value.find_first_of("/-", secondSeparator + 1) != std::string::npos) return false;

    const std::string first = value.substr(0, firstSeparator);
    const std::string second = value.substr(firstSeparator + 1, secondSeparator - firstSeparator - 1);
    const std::string third = value.substr(secondSeparator + 1);
    if (!isDigits(first) || !isDigits(second) || !isDigits(third)) return false;

    std::istringstream firstStream(first), secondStream(second), thirdStream(third);
    int a, b, c;
    firstStream >> a;
    secondStream >> b;
    thirdStream >> c;

    if (separator == '-' || third.size() != 4) {
        if (separator != '-' || first.size() != 4) return false;
        year = a;
        month = b;
        day = c;
    } else {
        day = a;
        month = b;
        year = c;
    }

    if (year < 1 || month < 1 || month > 12) return false;
    const bool leapYear = year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
    const int daysInMonth[] = {31, leapYear ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return day >= 1 && day <= daysInMonth[month - 1];
}

inline bool isValidDate(const std::string& value)
{
    int day, month, year;
    return parseDate(value, day, month, year);
}

inline std::string normalizeDate(const std::string& value)
{
    int day, month, year;
    if (!parseDate(value, day, month, year)) return value;
    std::ostringstream result;
    result << std::setfill('0') << std::setw(2) << day << "/"
           << std::setw(2) << month << "/" << std::setw(4) << year;
    return result.str();
}

inline int dateKey(const std::string& value)
{
    int day, month, year;
    if (!parseDate(value, day, month, year)) return 0;
    return year * 10000 + month * 100 + day;
}

inline bool todayDateKey(int& key)
{
    const std::time_t now = std::time(NULL);
    const std::tm* localNow = std::localtime(&now);
    if (localNow == NULL) return false;
    key = (localNow->tm_year + 1900) * 10000
        + (localNow->tm_mon + 1) * 100
        + localNow->tm_mday;
    return true;
}

inline std::string normalizeDateTime(const std::string& value)
{
    const std::size_t separator = value.find(' ');
    const std::string date = value.substr(0, separator);
    if (!isValidDate(date)) return value;
    if (separator == std::string::npos) return normalizeDate(date);
    return normalizeDate(date) + value.substr(separator);
}

}

#endif
