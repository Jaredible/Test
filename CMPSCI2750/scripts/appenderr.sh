#!/bin/bash
## append.sh
## appends $1 to $2

case $# in
2)	file="$2"	
	if [[ -e $file && -f $file && -w $file ]]
	then
		cat $1 >> $2
	else
		echo "Access problem for $file"
	fi
	;;
*)	echo "usage: $0 fromfile tofile"
esac
