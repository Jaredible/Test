#!/bin/bash

file=/tmp/logo.jpg
echo ${file:3}      # p/logo.jpg
echo ${file:3:5}    # p/log
echo ${file#*/}     # tmp/logo.jpg
base=${file##*/}    # logo.jpg (basename)

echo $base

echo ${file%/*}     #/tmp
echo ${file%%/l*}   #/tmp

echo ${base%.jpg}   # logo (root)
echo ${base%\.*}    # logo (root)

echo ${base##*\.}   # jpg (extension)
