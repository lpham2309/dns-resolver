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

class DNSResolver {
private:
    struct DNSHeader {
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
    std::string dns_server;
    uint16_t dns_port;
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

        DNSHeader header{};
        header.id = dist(gen);
        header.flags = byteSwap(0x0100);
        header.qdcount = byteSwap(1);
        header.ancount = 0;
        header.nscount = 0;
        header.arcount = 0;

        std::vector<uint8_t> header_data(sizeof(DNSHeader));
        std::memcpy(header_data.data(), &header, sizeof(DNSHeader));
        return header_data;

    }

    std::vector<uint8_t> createDNSQuestion(const std::string& domain) {
        DNSQuestion question;
        question.qname  = encodeDomainName(domain);
        question.qtype = byteSwap(1);
        question.qclass = byteSwap(1);

        std::vector<uint8_t> question_payload = question.qname;
        uint8_t typeclass[4];
        std::memcpy(typeclass, &question.qtype, 2);
        std::memcpy(typeclass + 2, &question.qclass, 2);
        question_payload.insert(question_payload.end(), typeclass, typeclass + 4);

        return question_payload;
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
    std::vector<std::string> parseDNSRecords(const std::vector<uint8_t> &response) {
        std::vector<std::string> ips;
        if (response.size() < sizeof(DNSHeader)) {
            return ips;
        }

        DNSHeader header;
        std::memcpy(&header, response.data(), sizeof(DNSHeader));

        uint16_t ancount = (header.ancount >> 8) | (header.ancount << 8);

        size_t pos = sizeof(DNSHeader);

        while (pos < response.size() && response[pos] != 0) {
            pos += response[pos] + 1;
        }
        pos += 5;

        for (uint16_t i = 0; i < ancount && pos + 12 <= response.size(); ++i) {
            if ((response[pos] & 0xC0) == 0xC0) {
                pos += 2;
            } else {
                while (pos < response.size() && response[pos] != 0) {
                    pos += response[pos] + 1;
                }
                pos++;
            }

            if (pos + 10 > response.size()) break;

            uint16_t type = (response[pos] << 8) | response[pos + 1];
            uint16_t datalen = (response[pos + 8] << 8) | response[pos + 9];
            pos += 10;

            if (type == 1 && datalen == 4 && pos + 4 <= response.size()) {
                char ip[INET_ADDRSTRLEN];
                struct in_addr addr;
                std::memcpy(&addr, &response[pos], 4);
                inet_ntop(AF_INET, &addr, ip, INET_ADDRSTRLEN);
                ips.push_back(ip);
            }

            pos += datalen;
        }

        return ips;
    }
public:
    DNSResolver(const std::string& server = "8.8.8.8", uint16_t port = 53): dns_server(server), dns_port(port) {}

    ~DNSResolver() {}

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
            auto dns_question = createDNSQuestion(domain);

            std::vector<uint8_t> query;
            query.insert(query.end(), dns_header.begin(), dns_header.end());
            query.insert(query.end(), dns_question.begin(), dns_question.end());

            if(sendto(socket_fd, query.data(), query.size(), 0, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) < 0) {
                throw std::runtime_error("Error sending query");
            }
            std::vector<uint8_t> response(2048);
            struct sockaddr_in from;
            socklen_t fromlen = sizeof(from);

            int received_payload = recvfrom(socket_fd, response.data(), response.size(), 0, reinterpret_cast<struct sockaddr*>(&from), &fromlen);

            if(received_payload < 0) {
                throw std::runtime_error("Error receiving response");
            }

            response.resize(received_payload);
            ips = parseDNSRecords(response);
        } catch(const std::exception& e) {
            std::cerr << "Error parsing DNS records" <<  domain << e.what() << std::endl;
        }
        close(socket_fd);
        return ips;
    };
};

int main() {
    try {
        DNSResolver resolver;
        std::string domain;

        while(true) {
            std::cout << "Please enter domain name to resolve (type 'exit' to quit): ";
            std::getline(std::cin, domain);

            if (domain == "exit") {
                break;
            }
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
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}