#!/bin/bash

function newer()
{
	[[ $1 -nt $2 ]]
}

# if no return used,
# exit status is exit status of last command executed

if newer t1.txt t2.txt
then
	echo "t1.txt is newer than t2.txt"
else
	echo "t2.txt is newer than t1.txt"
fi
