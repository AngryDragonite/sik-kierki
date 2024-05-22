import random

values = ['2','3','4','5','6','7','8','9','10','J','Q','K','A']
colors = ['C','D','H','S']
rozdania = ['1','2','3','4','5','6','7']
gracze = ['N','E','S','W']


all_cards = []

for value in values:
    for color in colors:
        all_cards.append(value + color)



def generate_cards():
    all_cards_copy = all_cards.copy()

    cards_string = random.choice(rozdania) + random.choice(gracze) + '\n'
    for j in range(0,4):
        for i in range(0,13):
            card = random.choice(all_cards_copy)
            all_cards_copy.remove(card)
            cards_string += card
        cards_string += '\n'
        
    return cards_string

f = open("cards.txt", "w")

for i in range(0,10):
    f.write(generate_cards())

f.close()

