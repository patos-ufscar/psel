#include "firewall.hpp"
#include <iostream>
#include <list>
#include <string>

int main() {
    Firewall fw;
    std::list<std::string> word_list = {"malware", "hack", "virus"};

    fw.ban_words(word_list);
    fw.ban_ip("10.0.0.50");

    std::cout << "Iniciando firewall (precisa de root)..." << std::endl;
    fw.run("tun0");
    return 0;
}
