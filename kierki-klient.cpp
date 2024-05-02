#include <iostream>
#include <string>
#include <endian.h>
#include <inttypes.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>

#include "common.h"

using std::string;
using std::cout;
using std::endl;
using std::cin;


void get_input(int argc, char* argv[], struct sockaddr_in &server_address, bool &ipv4, bool &ipv6, char &place, bool &automatic) {
    int c;
    string host;
    uint16_t port;

    while ((c = getopt(argc, argv, "h:p:46NESWa")) != -1) {
        switch (c) {
            case 'h':
                host = optarg;
                break;
            case 'p':
                port = read_port(optarg);
                break;
            case '4':
                ipv4 = true;
                break;
            case '6':
                ipv6 = true;
                break;
            case 'N':
                place = 'N';
                break;
            case 'E':
                place = 'E';
                break;
            case 'S':
                place = 'S';
                break;
            case 'W':
                place = 'W';
                break;
            case 'a':
                automatic = true;
                break;

        }
    }

    if (host.empty()) {
        fatal("Hostname is required.");
    }
    if (ipv4 and ipv6) {
        ipv6 = false;
    }

    if (place == 'X') {
        fatal("Place is required.");
    }

    server_address = get_server_address(host.c_str(), port);
}



int main(int argc, char* argv[]) {
    
    bool ipv4, ipv6, automatic;
    char place;
    struct sockaddr_in server_address;
    
    get_input(argc, argv, server_address, ipv4, ipv6, place, automatic);

}