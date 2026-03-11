#include "HttpRequest.h"
#include <sstream>

using namespace std;

HttpRequest parseHttpRequest(const string& raw_request){
    HttpRequest req;
    istringstream request_stream(raw_request);
    string line;

    // le a primeira linha (ex: GET /index.html HTTP/1.1)
    if(getline(request_stream, line)){
        istringstream line_stream(line);
        line_stream >> req.method >> req.path >> req.version;
    }

    // le os headers
    while(getline(request_stream, line) && line != "\r" && !line.empty()){
        size_t colon_pos = line.find(':');
        if(colon_pos != string::npos){
            string header_name = line.substr(0, colon_pos);
            string header_value = line.substr(colon_pos + 2);
            if(!header_value.empty() && header_value.back() == '\r'){
                header_value.pop_back();
            }
            req.headers[header_name] = header_value;
        }
    }

    // le o body
    ostringstream body_stream;
    body_stream << request_stream.rdbuf();
    req.body = body_stream.str();

    return req;
}