#include <iostream>
#include <unistd.h>
#include <string>
#include <fstream>
#include <signal.h>
#include <poll.h>
#include <endian.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <map>
#include <array>
#include <string.h>

#include "common.h"

using std::string;
using std::cout;
using std::endl;
using std::cin;
using std::ifstream;
using std::map;
using std::vector;
using std::array;
using std::to_string;

static bool finish = false;

static void catch_int(int sig) {
    finish = true;
    printf("signal %d catched so no new connections will be accepted\n", sig);
}

class Player {
    public:
        char side;
        int socket_fd;
        vector<array<string,13>> cards;
        vector<string> deal_cards;
        int temp_points;
        int total_points;

        Player(char side) {
            this->side = side;
            temp_points = 0;
            total_points = 0;
        }

        Player() {}

        bool has_card(const string& card, int rozdanie) {
            for (string card_: cards[rozdanie]) {
                if (card.compare(card_) == 0) {
                    return true;
                }
            }
            return false;
        }

        bool has_color(char color, int rozdanie) {
            for (string card : cards[rozdanie]) {
                if (card.back() == color) {
                    return true;
                }
            }
            return false;
        }

        void remove_card(const string& card, int rozdanie) {   
            for (int i = 0; i < 12; i++) {
                if (cards[rozdanie][i] == card) {
                    cards[rozdanie][i] = "empty";
                    break;
                }
            }
        }

        void parse_cards(const string& input) {
            //Tutaj nie do końca musi być 2, bo może być 10, a wtedy będzie np 27.
            //Trzeba to poprawić.
            
            array<string,13> cards;
            int card_number = 0;
            for (int i = 0; i < input.size(); i++) {
                string card;
                if (input[i] == '1' ) {
                    card = input.substr(i, 3);
                    i += 2;
                } else {
                    card = input.substr(i, 2);
                    i++;
                }
                cards[card_number] = card;
                card_number++;
            }
            this->cards.push_back(cards);
            this->deal_cards.push_back(input);
        }
};

class Server {
    
    public:
        map<char, Player> players;
        vector<char> who_starts;
        vector<int> deal_ids;
        vector<string> taken_lewa_history;
        int deal_count;
        struct sockaddr_in server_address;
        int socket_fd;


        Server() {
            players['N'] = Player('N');
            players['E'] = Player('E');
            players['S'] = Player('S');
            players['W'] = Player('W');
            deal_count = 0;
        }
        void handle_file_input(string file) {
            ifstream input_file(file);

            if (!input_file.is_open()) {
                fatal("cannot open file");
            }

            string input;
            uint64_t i = 0;

            while ((getline(input_file, input))) {
                uint8_t line_num = i % 5;
                
                if (line_num == 0) {
                    deal_ids.push_back(atoi(input.substr(0,1).c_str()));
                    who_starts.push_back(input[1]);
                    deal_count++;
                } else {
                    players[int_to_char(i%5)].parse_cards(input);
                }
                i++;
            }
        };

        void start(uint16_t port) {
            //socket
            socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (socket_fd < 0) {
                syserr("cannot create a socket");
            }

            server_address.sin_family = AF_INET; // IPv4
            server_address.sin_addr.s_addr = htonl(INADDR_ANY); // Listening on all interfaces.
            server_address.sin_port = htons(port);

            //bind
            if (bind(socket_fd, (struct sockaddr *) &server_address, (socklen_t) sizeof server_address) < 0) {
                syserr("bind");
            }

            //listening
            if (listen(socket_fd, CLIENTS) < 0) {
                syserr("listen");
            }
            cout << "socket\nbind\nlisten\n";
        }

        void send_taken(int lewa, const string& current_lewa_history, char winner) {
            string taken_msg = "TAKEN" + to_string(lewa) + current_lewa_history + winner +"\r\n";
            taken_lewa_history.push_back(taken_msg);
            cout << "sending TAKEN to every player\n";
        }

        void adjust_temp_points(char player, string& current_lewa_history, int rozdanie) {
            
            cout << "adjusting temporary points\n";
            current_lewa_history = "";

        }

        void adjust_total_points() {
            //Add temp points to total points
            cout << "adjusting total points\n";
        }

        void send_score_and_total() {
            
            //send SCORE - temp points 
            //send TOTAL - total points
            //set to 0 all temp points

            cout << "sending score and total\n";
        }

};

void send_wrong(int fd, int lewa) {
    string wrong_msg = "WRONG" + std::to_string(lewa + 1) + "\r\n";
    writen(fd, wrong_msg.c_str(), wrong_msg.size());
    cout << "sent wrong\n";
    //check for errors?
}


int main(int argc, char* argv[]) {
    
    //install_signal_handler(SIGINT, catch_int, SA_RESTART);
    
    uint16_t port;
    string file;
    unsigned time;

    get_input(argc, argv, port, file, time);

    Server server;
    server.handle_file_input(file);
    server.start(port);
    int socket_fd = server.socket_fd;


    //initialize pollfds struct array
    struct pollfd poll_descriptors[CLIENTS];

    poll_descriptors[0].fd = socket_fd;
    poll_descriptors[0].events = POLLIN;



    for (int i = 1; i < CLIENTS; ++i) {
        poll_descriptors[i].fd = -1;
        poll_descriptors[i].events = POLLIN;
    }

    size_t active_clients = 0;
    bool is_game_on = false;
    bool game_started = false;
    bool sent_deals = false;
    int lewa = 0;
    int rozdanie = 0;
    char last_joined;
    string current_lewa_history = "";
    char who_has_turn;
    char lewa_color;
    int lewa_round = 0;
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof client_address;


    do {
        if (!is_game_on) {
            cout << "waiting for players\n";
        }
        
        for (int i = 0; i < CLIENTS; ++i) {
            poll_descriptors[i].revents = 0;
        }

        // After Ctrl-C the main socket is closed.
        if (finish && poll_descriptors[0].fd >= 0) {
            close(poll_descriptors[0].fd);
            poll_descriptors[0].fd = -1;
        }

        int poll_status = poll(poll_descriptors, CLIENTS, 1000);

        if (poll_status == -1 ) {
            if (errno == EINTR) {
                printf("interrupted system call\n");
                close(socket_fd);
                //exit(1);
            }
            else {
                syserr("poll");
            }
        }

        else if (poll_status >= 0) {
            
            if (/*!finish && */(poll_descriptors[0].revents & POLLIN)) {
                int client_fd = accept(poll_descriptors[0].fd,
                                       (struct sockaddr *) &client_address,
                                       &client_address_len);
                set_timeout(client_fd, time);
                bool wrong_client = false;
                cout << "accepted connection from a new client\n";
                if (client_fd < 0) {
                    syserr("accept");
                }

                char buffer[6];

                //readn what the client has to say
                ssize_t read_len = readn(client_fd, &buffer, 6);
                if (read_len < 0) {
                    if (errno == EAGAIN) {
                        close(client_fd);
                        error("timeout");
                        continue;
                    } else {                    
                        syserr("readn");
                    }
                }
                cout << buffer << endl;

                char side = buffer[3];
                buffer[3] = 'X';
                
                if (strncmp(buffer, "IAMX\r\n", 6) != 0) {
                    error("wrong IAM message, %s", buffer);
                    close(client_fd);
                    wrong_client = true;
                }


                if (poll_descriptors[char_to_int(side)].fd != -1) {
                    string message = "BUSY" + get_busy_sides(poll_descriptors) + "\r\n";
                    writen(client_fd, message.c_str(), message.size());
                    close(client_fd);
                    cout << "sent " << message << endl;
                    wrong_client = true;
                }

                if (!wrong_client) {
                    poll_descriptors[char_to_int(side)].fd = client_fd;
                    poll_descriptors[char_to_int(side)].events = POLLIN;
                    last_joined = side;
                    active_clients++;
                    cout << "new client accepted\n";
                }
                
                if (!wrong_client and active_clients == 4) {
                    //Game hasnt started yet
                    if (!game_started) {
                        game_started = true;
                    } else {
                        //player joined in the middle of the game, after somebody left                        
                        string deal_msg = "DEAL"
                                          + to_string(server.deal_ids[rozdanie])
                                          + server.who_starts[rozdanie] 
                                          + server.players[last_joined].deal_cards[rozdanie] 
                                          + "\r\n";
                
                        writen(poll_descriptors[char_to_int(last_joined)].fd, deal_msg.c_str(), deal_msg.size());
                        //check for errors?
                        cout << "sent " << deal_msg << endl;

                        //send also every TAKEN history
                        for (string taken_lewa_msg : server.taken_lewa_history) {
                            writen(poll_descriptors[char_to_int(last_joined)].fd, taken_lewa_msg.c_str(), taken_lewa_msg.size());
                            //check for errors?
                        }
                    }
                    
                    is_game_on = true;
                }            
            } 

            //Handle connection requests
            if (is_game_on and !sent_deals and lewa == 0 and active_clients == 4) {
                //send DEAL to every client;
                who_has_turn = server.who_starts[rozdanie];
                string deal_msg_blueprint = "DEAL" + std::to_string(server.deal_ids[rozdanie]) + server.who_starts[rozdanie];
                for (int i = 1; i <= 4; i++) {
                    string deal_msg = deal_msg_blueprint + server.players[int_to_char(i)].deal_cards[rozdanie] + "\r\n";
                    writen(poll_descriptors[i].fd, deal_msg.c_str(), deal_msg.size());
                    cout << "sent " << deal_msg << endl;
                    //check for errors?
                }
                sent_deals = true;
                cout << "sent deals\n";
            }

            cout << "who has turn: " << who_has_turn << endl;
            bool player_serviced = false;

            for (int i = 1; i < CLIENTS; i++) {
                char temp_buff[1];
                if (poll_descriptors[i].fd != -1 and (poll_descriptors[i].revents & (POLLIN | POLLERR))) {
                    
                    ssize_t read_len = readn(poll_descriptors[i].fd, &temp_buff, 1);
                    if (read_len < 0) {
                        error("??? Bro u had one job..");
                        //disconnect?
                    } else if (read_len == 0) {
                        //somebody left
                        close(poll_descriptors[i].fd);
                        poll_descriptors[i].fd = -1;
                        active_clients--;
                        is_game_on = false;
                    } else {
                        send_wrong(poll_descriptors[i].fd, lewa);
                    }
                } else if (!player_serviced and who_has_turn == int_to_char(i)) {
                    if (!is_game_on) {
                        continue;
                    }
                    bool got_correct_answer = false;
                    char answer[12];

                    string trick_msg = "TRICK" + to_string(lewa) + current_lewa_history;
                    string card;
                    do {
                        writen(poll_descriptors[i].fd, trick_msg.c_str(), trick_msg.size());
                        cout << "sent " << trick_msg << " to " << who_has_turn << endl;

                        //Tutaj trzeba też ogarnąć to, że gracz może wyjść. Wtedy trzeba
                        //zbreakować
                        //Sending:
                        //TRICK0\r\n 8 min
                        //TRICK1310C10D10H10S\r\n 21 max

                        //receiving
                        //TRICK02S\r\n 10 min
                        //TRICK1310S\r\n 12 max

                        //Tutaj źle odczytuje. Trzeba zobaczyc w czym jest problem
                        //Co DOKŁADNIE wysyła i co DOKŁADNIE odbiera i czemu się nie zgadza
                        //W zeszycie jakiegoś quick fixa zrobić
                        ssize_t read_len = readn(poll_descriptors[i].fd, &answer, 12);
                        
                        cout << read_len << endl;
                        
                        if (read_len < 0) {
                            if (errno == EAGAIN) {
                                error("timeout");
                                continue;
                            } else {
                                syserr("readn");
                            }
                        } 

                        if (read_len == 0) {
                            //player has quit during his turn
                            close(poll_descriptors[i].fd);
                            poll_descriptors[i].fd = -1;
                            active_clients--;
                            is_game_on = false;
                            break;
                        }

                        card = validate_trick(lewa, answer, read_len);
                        cout << "card: " << card << endl;
                        if (card.compare("X") == 0) {
                            error("wrong TRICK, %s", answer);
                            send_wrong(poll_descriptors[i].fd, lewa);
                            continue;
                        }

                        //Now we know that the card is valid.
                        //We have to check if the player has such card
                        if (!server.players[who_has_turn].has_card(card, rozdanie)) {
                            error("wrong TRICK - player does not have such card");
                            send_wrong(poll_descriptors[i].fd, lewa);
                            continue;
                        }

                        //And also if specified card can be placed
                        if (lewa_round == 0) {
                            lewa_color = card.back();
                        } else if (lewa_round > 0 and lewa_color != card.back() and server.players[who_has_turn].has_color(lewa_color, rozdanie)) {
                            error("wrong TRICK - player can't place card of this color");
                            send_wrong(poll_descriptors[i].fd, lewa);
                            continue;
                        }

                        //Now we know that card can be placed - we can successfully complete card deployment procedure
                        got_correct_answer = true;
                        player_serviced = true;

                    } while (!got_correct_answer);

                    if (is_game_on) {
                        server.players[who_has_turn].remove_card(card,rozdanie);
                        current_lewa_history += card;
                        lewa_round++;
                        who_has_turn = get_next_player(who_has_turn);
                    }
                }
            }

            //Post-service logic
            if (is_game_on and lewa_round == 4) {
                
                lewa_round = 0;
                //choose lewa winner -> assign who_starts
                char winner = 'S';
                who_has_turn = winner;
                lewa++;
                string taken_msg = "TAKEN" + to_string(lewa) + current_lewa_history + winner +"\r\n";
                
                
                server.send_taken(lewa, current_lewa_history, winner);
                
                server.adjust_temp_points(winner, current_lewa_history, rozdanie); //Todo implement
            }

            if (is_game_on and lewa == 13) {
                lewa = 0;
                rozdanie++;
                sent_deals = false;
                //adjust total points
                server.adjust_total_points();
                server.send_score_and_total();
                //wyzerowac temp points
            }
            cout << "\nend of poll\n\n";
        }

    } while(rozdanie < server.deal_count);

    for (int i = 0; i < CLIENTS; i++) {
        if (poll_descriptors[i].fd != -1) {
            close(poll_descriptors[i].fd);
        }
    }

    return 0;

}