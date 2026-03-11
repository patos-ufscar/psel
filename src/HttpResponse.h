#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <map>

struct HttpResponse{
    int status_code;
    std::string status_message;
    std::map<std::string, std::string> headers;
    std::string body;

    // transforma o objeto de volta em uma string HTTP
    std::string toString();
};

#endif