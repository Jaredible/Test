#!/bin/bash
prime=(2 3 5 7 11 13 17)

function sum()
{
	local total=0;
	for i
		do let total+=$i     # (( total+=$i ))
	done
	echo $total	#return value
}


s=$( sum ${prime[@]} )    ## return it through command substitution
echo $s
