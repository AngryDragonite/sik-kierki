#include <iostream>
#include <unistd.h>
#include <string>

#include "common.h"

using std::string;
using std::cout;
using std::endl;
using std::cin;

void get_input(int argc, char *argv[], uint16_t &port, string &file, unsigned &time) {
    int c;
    string time_str;
    while ((c = getopt(argc, argv, "p:tf:")) != -1) {
        switch (c) {
            case 'f':
                file = optarg;
                break;
            case 'p':
                port = read_port(optarg);
                break;
            case 't':
                time_str =  atoi(optarg);
                break;
        }
    }

    if (port == 0) {
        fatal("Port number is required");
    }
    if (file.empty()) {
        fatal("File name is required");
    }
    if (time_str.empty()) {
        time = 5;
    }
}


int main(int argc, char* argv[]) {
    uint16_t port;
    string file;
    unsigned time;

    get_input(argc, argv, port, file, time);


}