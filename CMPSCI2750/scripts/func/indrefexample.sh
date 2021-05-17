#!/bin/bash

x=y;
y="abc";
echo $x     ## displays y
echo ${!x}  ## displays abc
