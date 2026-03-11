#include "HttpResponse.h"
#include <sstream>

using namespace std;

string HttpResponse::toString(){
    ostringstream out;
    out << "HTTP/1.1 " << status_code << " " << status_message << "\r\n";
    for(const auto& header : headers){
        out << header.first << ": " << header.second << "\r\n";
    }
    out << "\r\n" << body;
    return out.str();
}