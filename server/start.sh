# !/bin/bash
# Tester script for sockets using Netcat

echo "Clean make of aesdsocket"
make clean
make

cd ..
cd aesd-char-driver

echo "Clean make of asedchar driver"
make clean
make

echo "Clean unload and load of aesdchar driver"
sudo ./aesdchar_unload
sudo ./aesdchar_load

cd ..
cd server

echo "Starting aesdsocket"
./aesdsocket