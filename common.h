#ifndef COMMON_H
#define COMMON_H

#include <inttypes.h>
#include <stddef.h>
#include <sys/types.h>
#include <poll.h>
#include <vector>

#define CLIENTS 5

using std::string;
using std::vector;
using std::pair;

ssize_t	readn(int fd, void *vptr, size_t n);

ssize_t	writen(int fd, const void *vptr, size_t n);

uint16_t read_port(char const *string);

struct sockaddr_in get_server_address(char const *host, uint16_t port, int prottype);

void syserr(const char* fmt, ...);

void fatal(const char* fmt, ...);

void error(const char* fmt, ...);

void print_cards(const vector<pair<string, string>>& cards);

void install_signal_handler(int signal, void (*handler)(int), int flags);

void set_timeout(int fd, unsigned time);

int char_to_int(char c);

char int_to_char(int i);

void get_input(int argc, char *argv[], uint16_t &port, string &file, unsigned &time);

char get_next_player(char c);

string get_busy_sides(struct pollfd poll_descriptors[5]);

string validate_trick(int lewa, char* answer, ssize_t len);

void send_wrong(int fd_from, int fd, int lewa);

int card_to_int(const string& card);

void send_raport_msg(int server_fd, int fd_from, int fd_to, const string& msg);

vector<string> parse_cards_templ(const string& input);

void send_iam(int socket_fd, char place);

std::pair<int, std::vector<string>> parse_trick(const std::string& str);

string format_cards(const vector<string>& cards);

void parse_score(const string& str);

void send_message(int socket_fd, const string& message);

#endif // COMMON_H
