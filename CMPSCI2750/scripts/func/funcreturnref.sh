#!/bin/bash
prime=(2 3 5 7 11 13 17)

function sum()
{
	local p="$1[@]";     ## $p is "prime[@]"
	for i in "${!p}"     ## evalutes as ${prime[@]}
	do
		let $2+=$i   ## #2 is the symbol myTotal		
	done
}

myTotal=0
sum prime myTotal        ## passing two reference parameters
echo $myTotal



