#!/bin/bash

# Can redefine builtin commands
# but still have access to them using
## builtin commandName args   // invokes built-in commandName

function cd ()
{
	builtin cd "$1"
	/bin/ls -l
}

cd
pwd
echo "After our function cd"
builtin cd
pwd
echo "After builtin cd"
