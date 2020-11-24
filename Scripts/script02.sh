#!/bin/bash
OPTIONS="Hello Quit"
select opt in $OPTIONS; do
   if [ "$opt" = "Quit" ]; then
    echo done
    exit
   elif [ "$opt" = "Hello" ]; then
    echo Hello World
   else
    clear
    echo bad option
   fi
done

# ---------------------------
# EXTRA CREDIT - 12 Points
# ---------------------------
# If you completed script1.sh,then script2.sh can bring up to 8 more points. If
# you skip script1.sh and complete directly script2.sh, then it is worth up to 12 points.
# Modify your script so that it becomes a pure command interpreter and even the launching of a
# cellular automaton process is done via a script command. This means that your script could now
# launch and control multiple cellular automata. The command to launch a new cellular automaton
# would be
# • cell width height
# Each new cellular automation process would receive an index (starting with the first process at an
# index of 1) and display that index in its window’s title.
# All other commands to be interpreted by the script should now be prefixed by the index of the
# cellular automaton process to which they should be addressed:
# • 3: speedup means that Cellular automaton 3 should increase its speed;
# • 2: end means that Cellular automaton 2 should terminate execution;
# • etc.
# Invalid commands, including commands meant for a process that hasn’t been created or one that
# has already terminated, should be rejected.
# ---------------------------
