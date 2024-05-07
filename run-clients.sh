gnome-terminal --title="KlientN" -- ./kierki-klient -h 0.0.0.0 -p 2137 -N &

# Start the second C++ program in a new terminal
gnome-terminal --title="KlientE" -- ./kierki-klient -h 0.0.0.0 -p 2137 -E &

# Start the third C++ program in a new terminal
gnome-terminal --title="KlientS" -- ./kierki-klient -h 0.0.0.0 -p 2137 -S &

# Start the fourth C++ program in a new terminal
gnome-terminal --title="KlientW" -- ./kierki-klient -h 0.0.0.0 -p 2137 -W &
