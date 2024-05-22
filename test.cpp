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


struct Card {
    std::string value;
    char suit;

    void print() const {
        std::cout << value << suit << " ";
    }
};

vector<std::pair<char, int>> parseScore(const string& str) {
    vector<std::pair<char, int>> scores;
    size_t i = 5; // start after "SCORE"

    while (i < str.size()) {
        char c = str[i++];
        string num;
        while (i < str.size() && std::isdigit(str[i])) {
            num += str[i++];
        }
        scores.push_back({c, stoi(num)});
    }

    return scores;
}

int main() {
    
    string input = "SCOREN21E12093S23W0";

    vector<std::pair<char, int>> scores = parseScore(input);
    
    for (const auto& score : scores) {
        cout << score.first << " " << score.second << endl;
    }
}