#!/bin/bash
#title: ipcp.sh
#description: Displays your IPC resources and processes.
#usage: ./ipcp.sh

# Colors
red=`tput setaf 1`
green=`tput setaf 2`
yellow=`tput setaf 3`
blue=`tput setaf 4`
magenta=`tput setaf 5`
cyan=`tput setaf 6`
white=`tput setaf 7`

# Text modes
bold=`tput bold` # Select bold mode
smul=`tput smul` # Enable underline mode
rmul=`tput rmul` # Disable underline mode

# Other
reset=`tput sgr0` # Reset text format to the terminal's default

echo -e "\n${red}${bold}Message Queues${reset}"
ipcs -m | grep $USER
if [ $? -ne 0 ]
then
	echo "${white}${bold} --- None --- ${reset}"
fi

echo -e "\n${yellow}${bold}Shared Memory Segments${reset}"
ipcs -s | grep $USER
if [ $? -ne 0 ]
then
	echo "${white}${bold} --- None --- ${reset}"
fi

echo -e "\n${green}${bold}Semaphore Arrays${reset}"
ipcs -q | grep $USER
if [ $? -ne 0 ]
then
	echo "${white}${bold} --- None --- ${reset}"
fi

echo -e "\n${cyan}${bold}User Processes${reset}"
ps aux | grep $USER
if [ $? -ne 0 ]
then
	echo "${white}${bold} --- Something's wrong! --- ${reset}"
fi
echo -e
