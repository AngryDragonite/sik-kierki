CC = g++
CXXFLAGS = -Wall -Wextra -std=c++20
LFLAGS = 


TARGET1 = kierki-klient
TARGET2 = kierki-serwer

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(TARGET1).o common.o
	g++ $(CXXFLAGS) kierki-klient.o common.o -o kierki-klient
$(TARGET2): $(TARGET2).o common.o
	g++ $(CXXFLAGS) kierki-serwer.o common.o -o kierki-serwer

common.o: common.cpp
	g++ $(CXXFLAGS) -c -O2 common.cpp
kierki-klient.o: kierki-klient.cpp
	g++ $(CXXFLAGS) -c -O2 kierki-klient.cpp
kierki-serwer.o: kierki-serwer.cpp
	g++ $(CXXFLAGS) -c -O2 kierki-serwer.cpp



clean:
	rm -f $(TARGET1) $(TARGET2) *.o *~