#!/bin/bash

case $# in
0)	echo $0
	;;
1)	touch $1
	;;
*)	echo $*
esac
