#!/bin/bash

# Start the first C++ program in a new Windows Terminal window
wt -d . wsl ./kierki-klient -h 0.0.0.0 -p 2137 -N &

# Start the second C++ program in a new Windows Terminal window
wt -d . wsl ./kierki-klient -h 0.0.0.0 -p 2137 -E &

# Start the third C++ program in a new Windows Terminal window
wt -d . wsl ./kierki-klient -h 0.0.0.0 -p 2137 -S &

# Start the fourth C++ program in a new Windows Terminal window
wt -d . wsl ./kierki-klient -h 0.0.0.0 -p 2137 -W &