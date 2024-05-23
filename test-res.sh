#!/bin/sh
#the command to runn the code
gcc -O3 -march=native -mtune=native -pthread -o ex p3220120-p3220150-p3220227-pizza.c -pthread
./ex 100 10
