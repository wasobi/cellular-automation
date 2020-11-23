#!/bin/bash
# ---------------------------
# DIRECTIONS
# ---------------------------
# Get as arguments the width and height, and number of threads of the cellular automaton
# process to launch;
# • Setup a named pipe to communicate with the cellular automaton process, using the command
# syntax defined in the next subsection;
# • Launch a cellular automaton process, specifying the desired width and height;
# • Communicate with the CA, sending commands grabbed from the standard input.
# ---------------------------

mkfifo /tmp/cell

if [ ! -d /tmp/Version01 ]; then
  mkdir -p /tmp/Version01;
fi

# verify that the correct number of arguments was passed in
if [ "$#" = "2" ]
else
    echo "ERROR: not enough arguments"
    echo "Usage: $0 <path-to-directory> <filename>"
    exit
fi
