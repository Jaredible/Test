#!/bin/bash
## display random numbers until a user quits

go="yes"

while [[ "$go" == "yes" ]]
do
	echo "Your random number is $RANDOM!"
	echo "Would you like another one? [yes/no]:"
	read go
done
