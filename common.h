#ifndef COMMON_H
#define COMMON_H

#include <inttypes.h>
#include <stddef.h>
#include <sys/types.h>
#include <poll.h>
#include <vector>

#define CLIENTS 5

using std::string;

// 1) Send uint16_t, int32_t etc., not int.
//    The length of int is platform-dependent.
// 2) If we want to send a structure, we have to declare it
//    with __attribute__((__packed__)). Otherwise the compiler
//    may add a padding bewteen fields. In the following example
//    sizeof (data_pkt) is then 8, not 6.

typedef struct __attribute__((__packed__)) {
    uint16_t seq_no;
    uint32_t number;
} data_pkt;

typedef struct __attribute__((__packed__)) {
    uint64_t sum;
} response_pkt;




ssize_t	readn(int fd, void *vptr, size_t n);

ssize_t	writen(int fd, const void *vptr, size_t n);

uint16_t read_port(char const *string);

struct sockaddr_in get_server_address(char const *host, uint16_t port);

void syserr(const char* fmt, ...);

void fatal(const char* fmt, ...);

void error(const char* fmt, ...);

void print_cards(const std::vector<std::pair<std::string, std::string>>& cards);

void install_signal_handler(int signal, void (*handler)(int), int flags);

void set_timeout(int fd, unsigned time);

int char_to_int(char c);

char int_to_char(int i);

void get_input(int argc, char *argv[], uint16_t &port, string &file, unsigned &time);

char get_next_player(char c);

string get_busy_sides(struct pollfd poll_descriptors[5]);

string validate_trick(int lewa, char* answer, ssize_t len);


#endif // COMMON_H
