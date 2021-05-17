#!/bin/bash

prime=(2 3 5 7 11 13 17)

function displayArray()
{
	echo -n "(";

	## Now iterate over incoming args
	for el    ## iterates over positional parameter
	do
		echo -n " \"$el\" "
	done
	echo ")";
}

displayArray "${prime[@]}"	
