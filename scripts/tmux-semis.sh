#!/bin/bash 
tmux new-session -d
tmux send-keys "cd ~/vertx-semis/arduino" C-m
tmux send-keys "vim src/runtime.cpp" C-m
tmux new-window -d
tmux select-window -t 1
tmux send-keys "cd ~/vertx-semis/raspberry" C-m
tmux send-keys "vim runtime.py" C-m
tmux split-window -v
tmux resize-pane -D 10
tmux send-keys "cd ~/vertx-semis/raspberry/log" C-m
tmux send-keys "tail -n 15 -f system.log" C-m
tmux select-pane -t 0
tmux select-window -t 0
tmux -2 attach-session
