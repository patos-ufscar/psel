#ifndef LOGGER_H_
#define LOGGER_H_

#include <ctime>
#include <iostream>
#include <string>

class Logger {
public:
    static void info(const std::string& tag, const std::string& msg) {
        char b[20];
        std::time_t t = std::time(nullptr);
        std::strftime(b, sizeof b, "%H:%M:%S", std::localtime(&t));
        std::cout << "[" << b << "][" << tag << "] " << msg << std::endl;
    }
};

#endif // LOGGER_H_
