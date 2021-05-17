#!/bin/bash

first=${1%.*}
second=${2%.*}

a="$first"
b="$second"

echo $a
echo $b

c=$(( a / b ));

echo $c
