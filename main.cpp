//
// Created by Lam Pham on 1/1/25.
//

#include "main.h"
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <__random/random_device.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>

class DNSResolver {
private:
    std::string dns_server;
    uint16_t dns_port;

    struct DNDSHeader {
        uint16_t id;
        union {
            uint16_t flags;
            struct {
                uint16_t qr:1;
                uint16_t opcode:4;
                uint16_t aa:1;
                uint16_t tc:1;
                uint16_t rd:1;
                uint16_t ra:1;
                uint16_t z:1;
                uint16_t rcode:4;
            } bits;
        };
        uint16_t qdcount;
        uint16_t ancount;
        uint16_t nscount;
        uint16_t arcount;
    };
    struct DNSQuestion {
        std::vector<uint8_t> qname;
        uint16_t qtype;
        uint16_t qclass;
    };

    uint16_t byteSwap(uint16_t value) {
        return static_cast<uint16_t>((value << 8) | (value >> 8));
    }


    std::vector<uint8_t> createDNSHeader() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint16_t> dist(0, 65535);

        DNDSHeader header{};
        header.id = dist(gen);
        header.flags = byteSwap(0x0100);
        header.qdcount = byteSwap(1);
        header.ancount = 0;
        header.nscount = 0;
        header.arcount = 0;

        std::vector<uint8_t> header_data(sizeof(DNDSHeader));
        std::memcpy(header_data.data(), &header, sizeof(DNDSHeader));
        return header_data;

    }

    std::vector<uint8_t> createDNSQuestion(std::string& domain) {
        DNSQuestion question;
        question.qname  = encodeDomainName(domain);
        question.qtype = byteSwap(0x0100);
        question.qclass = byteSwap(0x0100);
    }

    std::vector<uint8_t> encodeDomainName(const std::string& domain) {
        std::vector<uint8_t> encoded;

        size_t pos = 0;
        size_t length = 0;

        encoded.push_back(0);

        for(char c : domain) {
            if (c=='.') {
                encoded[pos] = static_cast<uint8_t>(length);
                pos = encoded.size();
                encoded.push_back(0);
                length = 0;
            } else {
                encoded.push_back(static_cast<uint8_t>(c));
                length++;
            }
        }
        encoded[pos] = static_cast<uint8_t>(length);
        encoded.push_back(0);
        return encoded;
    }

public:
    std::vector<std::string> resolve(const std::string& domain) {
        std::vector<std::string> ips;

        int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd < 0) {
            std::cerr << "Error opening socket" << std::endl;
            return ips;
        }

        sockaddr_in server_address;
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(dns_port);

        inet_pton(AF_INET, dns_server.c_str(), &server_address.sin_addr);

        try {
            auto dns_header = createDNSHeader();
            auto dns_question = createDNSQuestion(header);
        }


    }
};


int main() {
    try {
        DNSResolver resolver;
        std::string domain;

        std::cout << "Please enter domain name to resolve: ";
        std::getline(std::cin, domain);

        auto domain_ips = resolver.resolve(domain);

        if(domain_ips.empty()) {
            std::cout << "No IP address found." << std::endl;
        } else {
            std::cout << "IP address found: " << std::endl;
            for(const auto& ip : domain_ips) {
                std::cout << ip << std::endl;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}