#!/bin/bash

function newer()
{
	if [[ $1 -nt $2 ]]      ## if file $1 is newer than file $2
	then
		return 0	## true
	else
		return 1	## false
	fi
}

# If no return is used, the exit code
# is the exit code of last statement executed

if newer t1.txt t2.txt
then
	echo "t1.txt is newer than t2.txt"
else
	echo "t2.txt is newer than t1.txt"
fi
