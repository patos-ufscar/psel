#include "FileHandler.h"
#include <fstream>
#include <sstream>

using namespace std;

FileHandler::FileHandler(const string& root) : doc_root(root){}

HttpResponse FileHandler::handle(const HttpRequest& req){
    HttpResponse res;
    
    string filepath = doc_root+(req.path == "/" ? "/index.html" : req.path);

    // sem Directory Traversal >)
    if(filepath.find("..") != string::npos){
        res.status_code = 403;
        res.status_message = "Forbidden";
        res.body = "<h1>403 - Forbidden</h1>";
        res.headers["Content-Length"] = to_string(res.body.size());
        res.headers["Content-Type"] = "text/html";
        return res;
    }

    ifstream file(filepath, ios::in | ios::binary);
    
    if(file){
        ostringstream contents;
        contents << file.rdbuf();
        file.close();

        res.status_code = 200;
        res.status_message = "OK";
        res.body = contents.str();

        string ext = filepath.substr(filepath.find_last_of(".") + 1);
        if(ext == "html") res.headers["Content-Type"] = "text/html";
        else if(ext == "css") res.headers["Content-Type"] = "text/css";
        else if(ext == "js") res.headers["Content-Type"] = "application/javascript";
        else res.headers["Content-Type"] = "text/plain";

    }else{
        res.status_code = 404;
        res.status_message = "Not Found";
        res.body = "<h1>404 - Not Found</h1>";
        res.headers["Content-Type"] = "text/html";
    }

    res.headers["Content-Length"] = to_string(res.body.size());
    res.headers["Connection"] = "close";

    return res;
}