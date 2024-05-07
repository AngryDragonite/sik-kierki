#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <iostream>
#include <vector>
#include <string>
#include <signal.h>
#include <poll.h>
#include <set>
#include <iostream>


#include "common.h"

using std::to_string;
using std::set;
using std::cout;
using std::endl;

uint16_t read_port(char const *string) {
    char *endptr;
    errno = 0;
    unsigned long port = strtoul(string, &endptr, 10);
    if (errno != 0 || *endptr != 0 || port > UINT16_MAX) {
        fatal("%s is not a valid port number", string);
    }
    return (uint16_t) port;
}

struct sockaddr_in get_server_address(char const *host, uint16_t port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *address_result;
    int errcode = getaddrinfo(host, NULL, &hints, &address_result);
    if (errcode != 0) {
        fatal("getaddrinfo: %s", gai_strerror(errcode));
    }

    struct sockaddr_in send_address;
    send_address.sin_family = AF_INET;   // IPv4
    send_address.sin_addr.s_addr =       // IP address
            ((struct sockaddr_in *) (address_result->ai_addr))->sin_addr.s_addr;
    send_address.sin_port = htons(port); // port from the command line

    freeaddrinfo(address_result);

    return send_address;
}

ssize_t readn(int fd, void *vptr, size_t n) {
    ssize_t nleft, nread;
    char *ptr;

    ptr = (char*) vptr;
    nleft = n;
    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0)
            return nread;     // When error, return < 0.
        else if (nread == 0)
            break;            // EOF

        nleft -= nread;
        ptr += nread;
    }
    return n - nleft;         // return >= 0
}

ssize_t writen(int fd, const void *vptr, size_t n) {
    ssize_t nleft, nwritten;
    const char *ptr;

    ptr = (char*) vptr;               // Can't do pointer arithmetic on void*.
    nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0)
            return nwritten;  // error

        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}

void syserr(const char* fmt, ...) {
    va_list fmt_args;
    int org_errno = errno;

    fprintf(stderr, "\tERROR: ");

    va_start(fmt_args, fmt);
    vfprintf(stderr, fmt, fmt_args);
    va_end(fmt_args);

    fprintf(stderr, " (%d; %s)\n", org_errno, strerror(org_errno));
    exit(1);
}

void fatal(const char* fmt, ...) {
    va_list fmt_args;

    fprintf(stderr, "\tERROR: ");

    va_start(fmt_args, fmt);
    vfprintf(stderr, fmt, fmt_args);
    va_end(fmt_args);

    fprintf(stderr, "\n");
    exit(1);
}

void error(const char* fmt, ...) {
    va_list fmt_args;
    int org_errno = errno;

    fprintf(stderr, "\tERROR: ");

    va_start(fmt_args, fmt);
    vfprintf(stderr, fmt, fmt_args);
    va_end(fmt_args);

    if (org_errno != 0) {
      fprintf(stderr, " (%d; %s)", org_errno, strerror(org_errno));
    }
    fprintf(stderr, "\n");
}

void print_cards(const std::vector<std::pair<std::string, std::string>>& cards) {
    for (int line = 0; line < 6; ++line) {
        for (const auto& card : cards) {
            std::string rank = card.first;
            std::string suit = card.second;
            std::string suitSymbol;
            if (suit == "C") {
                suitSymbol = "♠";
            } else if (suit == "S") {
                suitSymbol = "♣";
            } else if (suit == "D") {
                suitSymbol = "♦";
            } else if (suit == "H") {
                suitSymbol = "♥";
            }

            switch (line) {
                case 0:
                    std::cout << " _______ ";
                    break;
                case 1:
                    if (rank == "10") {
                        std::cout << "|" << rank << "     |";
                    } else std::cout << "|" << rank << "      |";
                    break;
                case 2:
                    std::cout << "|       |";
                    break;
                case 3:
                    std::cout << "|   " << suitSymbol << "   |";
                    break;
                case 4:
                    std::cout << "|       |";
                    break;
                case 5:
                    if (rank == "10") {
                        std::cout << "|_____" << rank << "|";
                    } else std::cout << "|______" << rank << "|";
                    break;
            }
        }
        std::cout << '\n';
    }
}

void install_signal_handler(int signal, void (*handler)(int), int flags) {
    struct sigaction action;
    sigset_t block_mask;

    sigemptyset(&block_mask);
    action.sa_handler = handler;
    action.sa_mask = block_mask;
    action.sa_flags = flags;

    if (sigaction(signal, &action, NULL) < 0 ){
        syserr("sigaction");
    }
}

void set_timeout(int socket_fd, unsigned MAX_WAIT) {
    //Ustawiamy wartość TIMEOUTu                                
    struct timeval to = {.tv_sec = MAX_WAIT, .tv_usec = 0};
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof to);
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &to, sizeof to);
}

char int_to_char(int i) {
    switch (i) {
        case 1:
            return 'N';
        case 2:
            return 'E';
        case 3:
            return 'S';
        case 4:
            return 'W';
        default:
            fatal("Invalid number");
            return 1;

    }
}

int char_to_int(char c) {

    switch (c) {
        case 'N':
            return 1;
            break;
        case 'E':
            return 2;
            break;
        case 'S':
            return 3;
            break;
        case 'W':
            return 4;
            break;
        default:
            fatal("Invalid character");
            return 1;
    }
}

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

char get_next_player(char c) {
    switch (c) {
        case 'N':
            return 'E';
        case 'E':
            return 'S';
        case 'S':
            return 'W';
        case 'W':
            return 'N';
        default:
            fatal("Invalid character");
            return 1;
    }
}

string get_busy_sides(struct pollfd poll_descriptors[5]) {
    string busy_sides = "";
    for (int i = 1; i <= 4; i++) {
        if (poll_descriptors[i].fd != -1) {
            busy_sides += int_to_char(i);
        }
    }
    return busy_sides;
}

string validate_trick(int lewa, char* answer, ssize_t len) {
    
    string valid_prefix = "TRICK" + to_string(lewa);
    string msg(answer);
    if (msg.compare(0, valid_prefix.size(), valid_prefix) != 0) {
        cout << "wrong prefix\n";
        return "X";
    }
    msg = msg.substr(valid_prefix.size());

    set<char> correct_vals = {'2','3','4','5','6','7','8','9','J','Q','K','A'};
    set<char> correct_colors = {'C', 'D', 'H', 'S'};
    set<char> correct_2nd = {'0', '1', '2', '3'};

    string card;

    if (msg.size() == 4) {
        if (!correct_vals.contains(msg[0]) or !correct_colors.contains(msg[1]) or msg[2] != '\r' or msg[3] != '\n') {
            cout << "wrong suffix\n";
            return "X";
        }
        card = msg.substr(0,2);
    } else if (msg.size() == 5) {
        if (msg[0] != '1' or !correct_2nd.contains(msg[1] or !correct_colors.contains(msg[2]) or msg[3] != '\r' or msg[4] != '\n')) {
            cout << "wrong suffix\n";
            return "X";
        }
        card = msg.substr(0,3);
    } else {
        cout << "something else wrong\n";
        return "X";
    }

    return card;
}