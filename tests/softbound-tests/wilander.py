#! /usr/bin/python

import sys, string, os, popen2

tests=[
#    " -4 0", ##unable to trigger these attacks
#    " -3 0", ##unable to trigger these attacks
#    " 1 0",  ##unable to trigger these attacks
#    " 2 0",  ##unable to trigger these attacks
    " 3 0",
#    " 4 0", ##unable to trigger these attacks
    " 5 0",
#    " 6 0", ##unable to trigger these attacks
    " -2 0",
    " -1 0"    
    " 7 0",
    " 8 0",
    " 9 0",
    " 10 0",
    " 11 0",
    " 12 0",
    " 13 0",
    " 14 0",
]


def run_command(command):

    print command
    exit_code  = os.system(command)
    if exit_code != 0:
        print "Error in running command:", command



# running wilander tests one by one 

for test in tests:

    run_command("./final " + test)

