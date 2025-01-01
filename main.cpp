//
// Created by Lam Pham on 1/1/25.
//

#include "main.h"
#include <iostream>
#include <string>
#include <vector>

class DNSResolver {
private:

};

int main() {
    try {
        DNSResolver resolver;
        std::string domain;

        std::cout << "Please enter domain name to resolve: ";
        std::getLine(std::cin, domain);

        resolver.resolve(domain);

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}