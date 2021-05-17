#!/bin/bash
## append.sh
## appends $1 to $2 or standard input to $1


case $# in
1)	cat >> "$1"
	;;
2)	cat $1 >> "$2"
	;;
*)	echo "usage: $0 [ fromfile ] tofile"
esac
