#!/bin/bash
# kipcs.cs 10/17/20
# Jared Diehl (jmddnb@umsystem.edu)
# ---------------------------------
# Deletes all your IPC resources.
# Note: Prints usage information for every IPC resource
#       group that has nothing to delete. Need to fix this.

# Deletes all your shared memory segments.
ipcs -m | grep `whoami` | awk '{ print $2 }' | xargs ipcrm shm
# Deletes all your semaphore arrays.
ipcs -s | grep `whoami` | awk '{ print $2 }' | xargs ipcrm sem
# Deletes all your message queues.
ipcs -q | grep `whoami` | awk '{ print $2 }' | xargs ipcrm msg
