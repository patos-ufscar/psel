#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <map>

struct HttpRequest{
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};

// converte a string bruta do socket em um objeto HttpRequest
HttpRequest parseHttpRequest(const std::string& raw_request);

#endif