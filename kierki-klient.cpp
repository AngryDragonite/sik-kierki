#include <iostream>
#include <string>
#include <endian.h>
#include <inttypes.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <poll.h>
#include <tuple>
#include <ctime>
#include <iomanip>


#include "common.h"

using std::string;
using std::cout;
using std::endl;
using std::cin;
using std::pair;
using std::to_string;

void get_input(int argc, char* argv[], struct sockaddr_in& server_address, bool &ipv4, bool &ipv6, char &place, bool &automatic) {
    int c;
    int ipprot = 0;
    place = 'X';
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
        ipprot = 4;
    } else if (ipv4) {
        ipprot = 4;
    } else if (ipv6) {
        ipprot = 6;
    }

    if (place == 'X') {
        fatal("Place is required.");
    }



    
    server_address = get_server_address(host.c_str(), port, ipprot);
}

void send_raport_msg(struct sockaddr_in& from_addr, struct sockaddr_in& to_addr, const string& msg) {

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

int main(int argc, char* argv[]) {
    
    bool ipv4, ipv6, automatic = false;
    char place;
    struct sockaddr_in server_address, self_address;
    
    get_input(argc, argv, server_address, ipv4, ipv6, place, automatic);

    // Create a socket.
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        syserr("cannot create a socket");
    }

    // Connect to the server.
    if (connect(socket_fd, (struct sockaddr *) &server_address,
                (socklen_t) sizeof(server_address)) < 0) {
        syserr("cannot connect to the server");
    }

    send_iam(socket_fd, place);
    send_raport_msg(self_address, server_address, "IAM" + string(1,place) + "\r\n");

    int points = 0;
    int id_rozdania;
    int lewa = 1;
    char current_lewa_color;
    char who_starts_rozdanie;
    bool look_for_card = false;
    vector<string> current_lewa_cards;
    vector<string> cards;
    vector<string> taken_lewa_history;
    string start_cards;
    string kbuff;
    string pbuff;
    struct pollfd poll_descriptors[2];
    string last_card;

    poll_descriptors[0].fd = socket_fd;
    poll_descriptors[0].events = POLLIN;
    poll_descriptors[1].fd = STDIN_FILENO;
    poll_descriptors[1].events = POLLIN;

    socklen_t addr_len = sizeof(self_address);

    if (getsockname(socket_fd, (struct sockaddr*) &self_address, &addr_len) < 0) {
        syserr("getsockname");
    }
    int count = 0;
    while (true) {
        poll_descriptors[0].revents = 0;
        poll_descriptors[1].revents = 0;

        int poll_status = poll(poll_descriptors, 2, -1);
        if (poll_status < 0) {
            syserr("poll");
        }

        if (poll_descriptors[0].revents & (POLLIN | POLLERR)) {
            char server_buf;
            ssize_t read_len = read(socket_fd, &server_buf, 1);
            if (read_len < 0) {
                syserr("read");
            }

            if (read_len == 0) {
                break;
            }
            kbuff.push_back(server_buf);
            
            if (kbuff[kbuff.size()-2] == '\r' and server_buf == '\n') {
                send_raport_msg(server_address, self_address, kbuff);
                kbuff.pop_back();
                kbuff.pop_back();
                if (count > 20) automatic = false;
                count++;
                if (kbuff.find("BUSY") == 0) {
                    if (!automatic) {
                        string message = "Place busy, list of busy places received: ";
                        for (int i = 4; i < kbuff.size(); i++) {
                            message.push_back(kbuff[i]);
                            message += ", ";
                        }
                        message.pop_back();
                        message.pop_back();
                        cout << message << endl;
                    } else {
                        string message = "Place busy, list of busy places received: ";
                        for (int i = 4; i < kbuff.size(); i++) {
                            message.push_back(kbuff[i]);
                            message += ", ";
                        }
                        message.pop_back();
                        message.pop_back();
                        cout << message << endl;
                    }
                    close(socket_fd);
                    return 0;
                } else if (kbuff.find("DEAL") == 0) {
                    //DEAL<typ rozdania><miejsce przy stole klienta wychodzącego jako pierwszy w rozdaniu><lista kart>
                    id_rozdania = kbuff[4];
                    who_starts_rozdanie = kbuff[5];
                    start_cards = kbuff.substr(6);
                    cards = parse_cards_templ(start_cards);
                    //New deal <typ rozdania>: staring place <miejsce przy stole klienta wychodzącego jako pierwszy w rozdaniu>, your cards: <lista kart>.
                    if (!automatic) {
                        string message = "New deal ";
                        message.push_back(id_rozdania);
                        message += " starting place ";
                        message.push_back(who_starts_rozdanie);
                        message += ", your cards: " + format_cards(cards);
                        
                        cout << message << endl;
                    } else {
                        string message = "New deal ";
                        message.push_back(id_rozdania);
                        message += " starting place ";
                        message.push_back(who_starts_rozdanie);
                        message += ", your cards: " + format_cards(cards);
                        
                        cout << message << endl;

                    }
                } else if (kbuff.find("TRICK") == 0) {

                    int lewa_c;
                    std::pair<int, vector<string>> parsed_info = parse_trick(kbuff);

                    lewa_c = parsed_info.first;
                    current_lewa_cards = parsed_info.second;

            
                    current_lewa_color = current_lewa_cards.size() == 0 ? 'X' : current_lewa_cards[0].back();
                    cout << "current color: " << current_lewa_color << endl;
                    if (!automatic) {
                        string message = "Trick: (" + std::to_string(lewa_c) + ") " + format_cards(current_lewa_cards);
                        message += "\nAvailable: " + format_cards(cards);
                        
                        cout << message << endl;
                        look_for_card = true;
                    } else {

                        string card;                                                
                        string message = "Trick: (" + std::to_string(lewa_c) + ") " + format_cards(current_lewa_cards);
                        message += "\nAvailable: " + format_cards(cards);
                        
                        cout << message << endl;

                        if (current_lewa_color != 'X' ) {
                            for (int i = 0; i < cards.size(); i++) {
                                if (cards[i].back() == current_lewa_color and (card.empty() or cards[i] < card)) {
                                    card = cards[i];
                                }
                            }
                        } 

                        if (card.empty()) {
                            for (int i = 0; i < cards.size(); i++) {
                                if (card.empty() or cards[i] < card) {
                                    card = cards[i];                                    
                                }
                            }  
                        }

                        string trick_msg = "TRICK" + std::to_string(lewa) + card + "\r\n";
                        send_message(socket_fd, trick_msg);
                        send_raport_msg(self_address, server_address, trick_msg);
                        cout << trick_msg;

                        last_card = card;
                        cards.erase(find(cards.begin(), cards.end(), card));

                    }
                } else if (kbuff.find("WRONG") == 0) {
                    //WRONG<numer lewy>
                    //Wrong message received in trick <numer lewy>.
                    if (!automatic) {
                        string message = "Wrong message received in trick " + kbuff.substr(5) + ".\n";
                        cout << message;
                    } else {
                        string message = "Wrong message received in trick " + kbuff.substr(5) + ".\n";
                        cout << message;
                    }
                    if (!last_card.empty()) {
                        cards.emplace_back(last_card);
                        last_card.clear();
                    }
                    automatic = false;
                } else if (kbuff.find("TAKEN") == 0) {
                    //TAKEN<numer lewy><lista kart><miejsce przy stole klienta biorącego lewę>
                    //A trick <numer lewy> is taken by <miejsce przy stole klienta biorącego lewę>, cards <lista kart>.
                    cout << kbuff << endl;
                    
                    if (!automatic) {
                        int lewa_c;
                        string winner = kbuff.substr(kbuff.size()-1,1);
                        kbuff.pop_back();

                        std::pair<int, vector<string>> parsed_info = parse_trick(kbuff);
                        lewa_c = parsed_info.first;
                        current_lewa_cards = parsed_info.second;
                        lewa++;


                        string message = "A trick " + std::to_string(lewa_c) + " is taken by " + winner + ", cards " + format_cards(current_lewa_cards) + ".\n";
                        cout << message;
                    } else {
                        int lewa_c;
                        string winner = kbuff.substr(kbuff.size()-1,1);
                        kbuff.pop_back();

                        std::pair<int, vector<string>> parsed_info = parse_trick(kbuff);
                        lewa_c = parsed_info.first;
                        current_lewa_cards = parsed_info.second;
                        lewa++;


                        string message = "A trick " + std::to_string(lewa_c) + " is taken by " + winner + ", cards " + format_cards(current_lewa_cards) + ".\n";
                        cout << message;
                        taken_lewa_history.push_back(message);
                    }
                    
                    
                } else if (kbuff.find("SCORE") == 0 or kbuff.find("TOTAL") == 0) {
                    taken_lewa_history.clear();
                    if (!automatic) {
                        parse_score(kbuff);
                        lewa = 1;
                        cards.clear();
                        current_lewa_cards.clear();
                    } else {
                        parse_score(kbuff);
                        lewa = 1;
                        cards.clear();
                        current_lewa_cards.clear();
                    }
                }

                kbuff.clear();
            }          
        }

        if (!automatic and (poll_descriptors[1].revents & (POLLIN | POLLERR))) {
            char user_buf[512];
            memset(user_buf, 0, sizeof(user_buf));

            ssize_t read_len = read(STDIN_FILENO, &user_buf, sizeof(user_buf));
            //post-user-read logic

            if (read_len < 0) {
                syserr("read");
            }

            //possible user input:
            // tricks
            // cards
            // !<card>
            string user_input = user_buf;         
            user_input.pop_back();

            if (user_input.compare("cards") == 0) {
                cout << "Your cards: ";
                for (int i = 0; i < cards.size(); i++) {
                    cout << cards[i] << ", ";
                }
                cout << endl;
            } else if (user_input.compare("tricks") == 0) {
                cout << "Taken tricks: \n";
                for (int i = 0; i < taken_lewa_history.size(); i++) {
                    cout << taken_lewa_history[i];
                }
                cout << endl;
            } else if (look_for_card and user_input.find("!") == 0) {
                string card = user_input.substr(1);
                bool can_place_card = current_lewa_color == 'X' ? true : card.back() == current_lewa_color;
                if (can_place_card and find(cards.begin(), cards.end(), card) != cards.end()) {
                    cards.erase(find(cards.begin(), cards.end(), card));
                    look_for_card = false;
                } else { cout << "sth wrong\n";}

                string trick_msg = "TRICK" + std::to_string(lewa) + card + "\r\n";
            
                send_message(socket_fd, trick_msg);

            }

            look_for_card = false;
        }
        //post-read logic
    }
    close(socket_fd);

}