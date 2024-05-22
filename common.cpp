#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <netdb.h>
#include <stddef.h>
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
#include <ctime>
#include <iomanip>


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

struct sockaddr_in get_server_address(char const *host, uint16_t port, int prottype = 0) {
    
    if (prottype == 0) {
        prottype = AF_UNSPEC;
    } else if (prottype == 4) {
        prottype = AF_INET;
    } else if (prottype == 6) {
        prottype = AF_INET6;
    } else {
        fatal("Invalid protocol type");
    }
    
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
    if (len) len = len;
    string valid_prefix = "TRICK" + to_string(lewa + 1);
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
    if (msg[2] == '\r' and msg[3] == '\n' and msg.size() == 5) {
        msg.pop_back();
    }

    if (msg.size() == 4) {
        if (!correct_vals.contains(msg[0]) or !correct_colors.contains(msg[1]) or msg[2] != '\r' or msg[3] != '\n') {
            cout << "wrong suffix, 4\n";
            return "X";
        }
        card = msg.substr(0,2);
    } else if (msg.size() == 5) {
        // if (msg[0] != '1' or !correct_2nd.contains(msg[1]) or !correct_colors.contains(msg[2]) or msg[3] != '\r' or msg[4] != '\n') {
        //     cout << "wrong suffix, 5\n";
        //     return "X";
        // }
        

        if (msg[0] != '1') {
            cout << "Condition failed: msg[0] != '1'\n";
            return "X";
        }
        if (!correct_2nd.contains(msg[1])) {
            cout << "Condition failed: !correct_2nd.contains(msg[1])\n";
            return "X";
        }
        if (!correct_colors.contains(msg[2])) {
            cout << "Condition failed: !correct_colors.contains(msg[2])\n";
            return "X";
        }
        if (msg[3] != '\r') {
            cout << "Condition failed: msg[3] != '\\r'\n";
            return "X";
        }
        if (msg[4] != '\n') {
            cout << "Condition failed: msg[4] != '\\n'\n";
            return "X";
        }



        card = msg.substr(0,3);
    } else {
        cout << "something else wrong\n";
        return "X";
    }

    return card;
}


void send_wrong(int fd_from, int fd, int lewa) {
    string wrong_msg = "WRONG" + std::to_string(lewa + 1) + "\r\n";
    writen(fd, wrong_msg.c_str(), wrong_msg.size());
    //cout << "sent wrong\n";
    send_raport_msg(fd_from, fd_from, fd, wrong_msg);
    //check for errors?
}

int card_to_int(const string& card) {
    if (card[0] == '1') {
        return 10;
    }

    char val = card[0];

    switch (val) {
        case '2':
            return 2;
        case '3':
            return 3;
        case '4':
            return 4;
        case '5':
            return 5;
        case '6':
            return 6;
        case '7':
            return 7;
        case '8':
            return 8;
        case '9':
            return 9;
        case 'J':
            return 11;
        case 'Q':
            return 12;
        case 'K':
            return 13;
        case 'A':
            return 14;
    }

    return 0;
}

void send_raport_msg(int server_fd, int fd_from, int fd_to, const string& msg) {
    struct sockaddr_in from_addr, to_addr;
    socklen_t from_len = sizeof(from_addr), to_len = sizeof(to_addr);
    if (server_fd == fd_from) {
    
        if (getsockname(fd_from, (struct sockaddr*)&from_addr, &from_len) < 0) {
            error("getsockname");
        } 
        if (getpeername(fd_to, (struct sockaddr*)&to_addr, &to_len) < 0) {
            error("getpeername");
        }
    } else {
        if (getpeername(fd_from, (struct sockaddr*)&from_addr, &from_len) < 0) {
            error("getpeername");
        } 
        if (getsockname(fd_to, (struct sockaddr*)&to_addr, &to_len) < 0) {
            error("getsockname");
        }
    }

    char const *from_ip = inet_ntoa(from_addr.sin_addr);
    uint16_t from_port = ntohs(from_addr.sin_port);

    char const *to_ip = inet_ntoa(to_addr.sin_addr);
    uint16_t to_port = ntohs(to_addr.sin_port);

    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);

    string raport_msg = string("[") + from_ip + ":" + to_string(from_port) 
                            + ", " + to_ip + ":" + to_string(to_port) 
                            + ", ";
    
    cout << raport_msg << std::put_time(now, "%Y-%m-%dT%H:%M:%S.000") << "] " << msg;
}



vector<string> parse_cards_templ(const string& input) {            
    vector<string> cards;
    for (unsigned long i = 0; i < input.size(); i++) {
        string card;
        if (input[i] == '1' ) {
            card = input.substr(i, 3);
            i += 2;
        } else {
            card = input.substr(i, 2);
            i++;
        }
        cards.push_back(card);
    }

    return cards;
}


void send_iam(int socket_fd, char place) {
    char msg[6];
    msg[0] = 'I';
    msg[1] = 'A';
    msg[2] = 'M';
    msg[3] = place;
    msg[4] = '\r';
    msg[5] = '\n';

    ssize_t write_len = write(socket_fd, &msg, 6);
    if (write_len < 0) {
        syserr("write");
    }
}

std::pair<int, std::vector<string>> parse_trick(const std::string& str) {
    std::vector<string> cards;
    int i = str.size() - 1;

    // Parse cards from the end of the string
    while (i > 6) { // 5 is the length of "TRICK" plus one digit
        string card(1, str[i]); // assign suit before decrementing i
        i--;
        if (str[i] == '0') { // Handle 10
            card = str.substr(i - 1, 2) + card;
            i -= 2;
        } else {
            card = string(1, str[i]) + card;
            i--;
        }
        cards.push_back(card);
    }
    

    vector<string> cards_rev;

    for (int i = cards.size() - 1; i >= 0; i--) {
        cards_rev.push_back(cards[i]);
    }

    // Parse the number
    int num = std::stoi(str.substr(5, i - 4));
    return {num, cards_rev};
}

string format_cards(const vector<string>& cards) {
    string msg;

    for (int i = 0; i < cards.size(); i++) {
        msg += cards[i] + ", ";
    }
    if (msg.size() > 2) {
        msg.pop_back();
        msg.pop_back();
    }

    return msg;
}


void parse_score(const string& str) {
    size_t i = 5; // start after "SCORE"
    string message = "The ";
    message += (str.substr(0, 5) == "TOTAL" ? "total " : "");
    message += "scores are:\n";
    cout << message;
    while (i < str.size()) {
        char c = str[i++];
        string num;
        while (i < str.size() && std::isdigit(str[i])) {
            num += str[i++];
        }
        cout << string(1, c) << " | " << num << endl;
    }
}


void send_message(int socket_fd, const string& message) {
    char message_c[message.size()];
    strncpy(message_c, message.c_str(), message.size());
    ssize_t write_len = write(socket_fd, &message_c, message.size());
    if (write_len < 0 ) {
        syserr("write");
    }
}