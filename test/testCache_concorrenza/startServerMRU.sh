#!/bin/bash

RED="\033[0;31m"
DEFAULT_COLOR="\033[0m"

clientDir=./build/client
serverDir=./build/server
sockFile=cs_sock
configFile=test/config/testCache_concorrenzaMRU.txt

echo -e "${RED}Avvio il server${DEFAULT_COLOR}"
valgrind $serverDir $configFile
