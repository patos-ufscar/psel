#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include "HttpRequest.h"
#include "HttpResponse.h"
#include <string>

class FileHandler{
private:
    std::string doc_root;

public:
    FileHandler(const std::string& root);
    HttpResponse handle(const HttpRequest& req);
};

#endif