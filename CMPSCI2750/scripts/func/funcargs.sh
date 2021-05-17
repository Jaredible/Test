#!/bin/bash

#function compare(str1, str2)   ## wrong

function compare()
{
	local str1="$1"
	local str2="$2"
	if [[ $str1 == $str2 ]]
		then echo 0;
	elif [[ $str1 > $str2 ]]
		then echo 1;
	else
		echo -1;
	fi
}

compare dog cat
compare dog dog
compare cat dog
