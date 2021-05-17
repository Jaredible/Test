#!/bin/bash

# When you type ^C, you send an interrupt signal to a running script
# Default response on getting a interrupt is to terminate
# What if we want to change that?

# Format:
# trap command sig
# man 7 signal for list of signals

trap "echo Aborting $0 through interrupt; exit 1;" 2 15
#trap "echo No thanks, I dont feel like it" 2

while :
do
	echo "Press [CTRL+C] to exit"
	sleep 1
done




