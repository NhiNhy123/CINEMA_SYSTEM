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

inline bool isFutureDateTime(const std::string& date, const std::string& time)
{
    const int targetDateKey = dateKey(date);
    if (targetDateKey == 0 ||
        (time.size() != 5 && time.size() != 8) ||
        time[2] != ':' ||
        (time.size() == 8 && time[5] != ':')) {
        return false;
    }

    const std::string hourText = time.substr(0, 2);
    const std::string minuteText = time.substr(3, 2);
    const std::string secondText = time.size() == 8 ? time.substr(6, 2) : "00";
    if (!isDigits(hourText) || !isDigits(minuteText) || !isDigits(secondText)) return false;

    const int hour = (hourText[0] - '0') * 10 + (hourText[1] - '0');
    const int minute = (minuteText[0] - '0') * 10 + (minuteText[1] - '0');
    const int second = (secondText[0] - '0') * 10 + (secondText[1] - '0');
    if (hour > 23 || minute > 59 || second > 59) return false;

    const std::time_t now = std::time(NULL);
    const std::tm* localNow = std::localtime(&now);
    if (localNow == NULL) return false;

    const int currentDateKey = (localNow->tm_year + 1900) * 10000
                             + (localNow->tm_mon + 1) * 100
                             + localNow->tm_mday;
    if (targetDateKey != currentDateKey) return targetDateKey > currentDateKey;

    const int targetSeconds = hour * 3600 + minute * 60 + second;
    const int currentSeconds = localNow->tm_hour * 3600
                             + localNow->tm_min * 60
                             + localNow->tm_sec;
    return targetSeconds > currentSeconds;
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
