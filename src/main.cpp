#include "ServerSocket.h"
#include <iostream>

using namespace std;

int main(){
    const string PORT = "8080";
    const string DOC_ROOT = "./www";

    cout << "Iniciando o servidor..." << endl;
    
    ServerSocket server(PORT, DOC_ROOT);
    server.start();

    return 0;
}