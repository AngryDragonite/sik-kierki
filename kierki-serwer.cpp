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
#include <endian.h>

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
        vector<vector<string>> cards;
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
            for (int i = 0; i < 13; i++) {
                if (cards[rozdanie][i] == card) {
                    cards[rozdanie][i] = "empty";
                    break;
                }
            }
        }

        void parse_cards(const string& input) {            
            vector<string> cards = parse_cards_templ(input);
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

            int yes = 1;
            if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
                syserr("setsockopt");
            }

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


        void tcp_multicast(const string& message, struct pollfd poll_descriptors[CLIENTS]) {
            char message_buf[message.size()];
            strncpy(message_buf, message.c_str(), message.size());
            
            for (int i = 1; i < CLIENTS; i++) {
                ssize_t write_len = write(poll_descriptors[i].fd, &message_buf, message.size());
                if (write_len < 0) {
                    syserr("write");
                }
                send_raport_msg(socket_fd, socket_fd, poll_descriptors[i].fd, message);
                //check for errors?
            }
        }

        void send_taken(int lewa, const string& current_lewa_history, char winner, struct pollfd poll_descriptors[CLIENTS]) {
            string taken_msg = "TAKEN" + to_string(lewa) + current_lewa_history + winner +"\r\n";
            taken_lewa_history.push_back(taken_msg);
            
            tcp_multicast(taken_msg, poll_descriptors);

            //cout << "sending TAKEN to every player\n";
        }

        void adjust_temp_points(char player, string& current_lewa_history, int rozdanie, int lewa) {
            
            int points = 0;
            int deal_type = deal_ids[rozdanie];

            array<string, 4> lewa_cards;
            int card_id = 0;
            for (unsigned long i = 0; i < current_lewa_history.size(); i++) {
                if (current_lewa_history[i] == '1' ) {
                    lewa_cards[card_id] = current_lewa_history.substr(i, 3);
                    i += 2;
                } else {
                    lewa_cards[card_id] = current_lewa_history.substr(i, 2);
                    i++;
                }
                card_id++;
            }

            switch (deal_type) {
                case 1:
                    points++;
                    break;
                case 2:
                    //Adjust points for deal 2
                    for (string card : lewa_cards) {
                        if (card.back() == 'H') {
                            points++;
                        }
                    }
                    break;
                case 3:
                    for (string card : lewa_cards) {
                        if (card[0] == 'Q') {
                            points += 5;
                        }
                    }
                    break;
                case 4:
                    //Adjust points for deal 4
                    for (string card : lewa_cards) {
                        if (card[0] == 'K' or card[0] == 'J') {
                            points += 2;
                        }
                    }
                    break;
                case 5:
                    for (string card : lewa_cards) {
                        if (card[0] == 'K' and card[1] == 'H') {
                            points += 18;
                        }
                    }
                    break;
                case 6:
                    if (lewa == 6 or lewa == 12) {
                        points += 10;
                    }
                    break;
                case 7:
                    //Adjust points for deal 7
                    points++;
                    for (string card : lewa_cards) {
                        if (card.back() == 'H') {
                            points++;
                        }
                        if (card[0] == 'Q') {
                            points += 5;
                        }
                        if (card[0] == 'Q') {
                            points += 5;
                        }
                        if (card[0] == 'K' or card[0] == 'J') {
                            points += 2;
                        }
                        if (card[0] == 'K' and card[1] == 'H') {
                            points += 18;
                        }
                    }
                    if (lewa == 6 or lewa == 12) {
                        points += 10;
                    }
                    break;
            }

            players[player].temp_points += points;
            current_lewa_history.clear();

        }

        void adjust_total_points() {
            //Add temp points to total points

            char player_sides[4] = {'N', 'E', 'S', 'W'};

            for (auto side : player_sides) {
                players[side].total_points += players[side].temp_points;
            }

            cout << "adjusting total points\n";
        }

        void send_score_and_total(struct pollfd poll_descriptors[CLIENTS]) {
            
            string score_msg = "SCORE";

            for (auto player = players.begin(); player != players.end(); ++player) {
                score_msg += player->second.side + to_string(player->second.temp_points);
            }
            score_msg += "\r\n";

            tcp_multicast(score_msg, poll_descriptors);

            string total_msg = "TOTAL";
            for (auto player = players.begin(); player != players.end(); ++player) {
                total_msg += player->second.side + to_string(player->second.total_points);
            }
            total_msg += "\r\n";

            tcp_multicast(total_msg, poll_descriptors);

            for (std::pair<char,Player> player : players) {
                player.second.temp_points = 0;
            }
            
            cout << "sending score and total\n";
        }

};


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
    char lewa_winner;
    int lewa_highest_card = 0;
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

        int poll_status = poll(poll_descriptors, CLIENTS, 50);

        if (poll_status == -1 ) {
            if (errno == EINTR) {
                printf("interrupted system call\n");
                close(socket_fd);
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
                //cout << "accepted connection from a new client\n";
                if (client_fd < 0) {
                    syserr("accept");
                }

                char buffer[6];

                //read what the client has to say
                ssize_t read_len = read(client_fd, &buffer, 6);
                if (read_len < 0) {
                    if (errno == EAGAIN) {
                        error("timeout");
                        ssize_t write_len = write(client_fd, &read_len,0 );
                        if (write_len < 0) {
                            error("write");
                        }
                        close(client_fd);

                        continue;
                    } else {                    
                        error("read");
                    }
                }
                
                string msg(buffer);
                send_raport_msg(socket_fd, client_fd, socket_fd, msg);

                char side = buffer[3];
                buffer[3] = 'X';
                
                if (strncmp(buffer, "IAMX\r\n", 6) != 0) {
                    error("wrong IAM message, %s", buffer);
                    ssize_t write_len = write(client_fd, &side, 0 );
                    if (write_len < 0) {
                        error("write");
                    }
                    close(client_fd);
                    wrong_client = true;
                }


                if (poll_descriptors[char_to_int(side)].fd != -1) {
                    string message = "BUSY" + get_busy_sides(poll_descriptors) + "\r\n";
                    send_message(client_fd, message);
                    send_raport_msg(socket_fd, socket_fd, client_fd, message);
                    close(client_fd);
                    //cout << "sent " << message << endl;
                    wrong_client = true;
                }

                if (!wrong_client) {
                    poll_descriptors[char_to_int(side)].fd = client_fd;
                    poll_descriptors[char_to_int(side)].events = POLLIN;
                    last_joined = side;
                    active_clients++;
                    //cout << "new client accepted\n";
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
                
                        send_message(client_fd, deal_msg);
                        //cout << "sent " << deal_msg << endl;
                        send_raport_msg(socket_fd, socket_fd, poll_descriptors[char_to_int(last_joined)].fd, deal_msg);

                        //send also every TAKEN history
                        for (string& taken_lewa_msg : server.taken_lewa_history) {
                            send_message(client_fd, taken_lewa_msg);
                            send_raport_msg(socket_fd, socket_fd, poll_descriptors[char_to_int(last_joined)].fd, taken_lewa_msg);
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
                    send_message(poll_descriptors[i].fd, deal_msg);
                    send_raport_msg(socket_fd, socket_fd, poll_descriptors[i].fd, deal_msg);
                    //cout << "sent " << deal_msg << endl;
                    //check for errors?
                }
                sent_deals = true;
                //cout << "sent deals\n";
            }

            //cout << "who has turn: " << who_has_turn << endl;
            bool player_serviced = false;

            for (int i = 1; i < CLIENTS; i++) {
                char temp_buff[1];
                if (poll_descriptors[i].fd != -1 and (poll_descriptors[i].revents & (POLLIN | POLLERR))) {
                    
                    ssize_t read_len = read(poll_descriptors[i].fd, &temp_buff, 1);
                    if (read_len <= 0) {
                        //somebody left
                        close(poll_descriptors[i].fd);
                        poll_descriptors[i].fd = -1;
                        active_clients--;
                        is_game_on = false;
                    } else {
                        send_wrong(socket_fd, poll_descriptors[i].fd, lewa);
                    }
                } else if (!player_serviced and who_has_turn == int_to_char(i)) {
                    if (!is_game_on) {
                        continue;
                    }
                    bool got_correct_answer = false;
                    char answer[12];

                    string trick_msg = "TRICK" + to_string(lewa + 1) + current_lewa_history + "\r\n";
                    string card;
                    do {
                        send_message(poll_descriptors[i].fd, trick_msg);
                        send_raport_msg(socket_fd, socket_fd, poll_descriptors[i].fd, trick_msg);

                        ssize_t read_len = read(poll_descriptors[i].fd, &answer, 12);
                                                
                        if (read_len < 0) {
                            if (errno == EAGAIN) {
                                error("timeout");
                                continue;
                            } else {
                                error("read");
                                close(poll_descriptors[i].fd);
                                poll_descriptors[i].fd = -1;
                                active_clients--;
                                is_game_on = false;
                                break;
                            }
                        } 

                        send_raport_msg(socket_fd, poll_descriptors[i].fd, socket_fd, string(answer));

                        if (read_len == 0) {
                            //player has quit during his turn
                            close(poll_descriptors[i].fd);
                            poll_descriptors[i].fd = -1;
                            active_clients--;
                            is_game_on = false;
                            break;
                        }

                        card = validate_trick(lewa, answer, read_len);

                        if (card.compare("X") == 0) {
                            error("wrong TRICK, %s", answer);
                            send_wrong(socket_fd, poll_descriptors[i].fd, lewa);
                            continue;
                        }

                        //Now we know that the card is valid.
                        //We have to check if the player has such card
                        if (!server.players[who_has_turn].has_card(card, rozdanie)) {
                            error("wrong TRICK - player does not have such card");
                            send_wrong(socket_fd, poll_descriptors[i].fd, lewa);
                            continue;
                        }

                        //And also if specified card can be placed
                        if (lewa_round == 0) {
                            lewa_color = card.back();
                        } else if (lewa_round > 0 and lewa_color != card.back() and server.players[who_has_turn].has_color(lewa_color, rozdanie)) {
                            error("wrong TRICK - player can't place card of this color");
                            for (auto card : server.players[who_has_turn].cards[rozdanie]) {
                                if (card.compare("empty") != 0 ) {
                                    cout << card << " ";
                                }
                            }
                            cout << endl;

                            send_wrong(socket_fd, poll_descriptors[i].fd, lewa);
                            continue;
                        }

                        //Now we know that card can be placed - we can successfully complete card deployment procedure
                        got_correct_answer = true;
                        player_serviced = true;

                    } while (!got_correct_answer);

                    if (is_game_on) {
                        if (lewa_round == 0) {
                            lewa_winner = who_has_turn;
                            lewa_highest_card = card_to_int(card);
                            
                        } else if (lewa_color == card.back() and card_to_int(card) > lewa_highest_card) {
                            
                            lewa_winner = who_has_turn;
                            lewa_highest_card = card_to_int(card);
                        }
                        
                        cout << "highest card: " << lewa_highest_card << endl;
                        cout << "lewa winner: " << lewa_winner << endl;

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
                who_has_turn = lewa_winner;
                cout << "lewa winner: " << lewa_winner << endl;
                lewa++;           
                server.send_taken(lewa, current_lewa_history, lewa_winner, poll_descriptors);
                server.adjust_temp_points(lewa_winner, current_lewa_history, rozdanie, lewa);
            }

            if (is_game_on and lewa == 13) {
                lewa = 0;
                rozdanie++;
                sent_deals = false;
                server.adjust_total_points();
                server.send_score_and_total(poll_descriptors);
            }
        }

    } while (rozdanie < server.deal_count);


    for (int i = 1; i < CLIENTS; i++) {
        if (poll_descriptors[i].fd != -1) {
            send_message(poll_descriptors[i].fd, "");
            close(poll_descriptors[i].fd);
        }
    }

    close(socket_fd);
    return 0;

}