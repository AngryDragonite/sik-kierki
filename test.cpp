#include <iostream>
#include <string>
#include <array>
#include <vector>

using std::string;
using std::array;
using std::vector;
using std::cout;
using std::endl;

array<string,13> parse_cards(string input) {
    
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
    return cards;
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


int main() {
    
    string input = "ASKC7SQD2C5S4DJC6C9DQC4SAD";
    
    array<string,13> cards = parse_cards(input);
    
    vector<std::pair<std::string, std::string>> cards_vector;

    for (int i = 0; i < 13; i++) {
        string card = cards[i];
        string rank;
        string suit;

        if (card.size() == 3) {
            rank = card.substr(0, 2);
            suit = card.substr(2, 1);
        } else {
            rank = card.substr(0, 1);
            suit = card.substr(1, 1);
        }
        cards_vector.push_back(std::make_pair(rank, suit));
    }

    print_cards(cards_vector);
    // for (int i = 0; i < 13; i++) {
    //     std::cout << cards_vector[i].first << cards_vector[i].second  << std::endl;
    // }

}