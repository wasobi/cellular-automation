#!/bin/bash
WIDTH=$1
HEIGHT=$2
THREADS=$3
# verify that the correct number of arguments was passed in
if [ "$#" -ne 3 ]
then
    echo "ERROR: not enough arguments"
    echo "---------------------------"
    echo "Usage: $0 <width> <height> <threads>"
    exit
else
    ./cell_v1 $WIDTH $HEIGHT $THREADS
fi

# pipe commands into the program
PIP=/tmp/cell
if [[ ! -p $PIP ]]; then
    mkfifo $PIP
fi
while [[ $CMD != "end" ]]; do
    echo " Enter a command:"
    echo "[rule <#>] | [color on] | [color off] | [speedup] | [slowdown] | [end]"
    read CMD
    case $CMD in
        "rule" & [1-4])
        echo $CMD > $PIP
        ;;
        "color on")
        echo $CMD > $PIP
        ;;
        "color off")
        echo $CMD > $PIP
        ;;
        "speedup")
        echo $CMD > $PIP
        ;;
        "slowdown")
        echo $CMD > $PIP
        ;;
        "end"
        echo $CMD > $PIP
        ;;
        *)
        echo "ERROR: unknown command, please try again."
        echo "Usage: {rule <#>|color on|color off|speedup|slowdown|end}"
        ;;
    esac
done

# if [[ "$#"]];
#     then
#         echo $* > $PIPE
# else


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
