#/bin/bash
###############################
#
# Script to test WPA, change variables and options for testing
#
##############################

TNAME=rc
###########SET variables and options when testing using executable file
EXEFILE=$PTABIN/rc    ### Add the tools here for testing
FLAGS="-rc-detail"  ### Add the FLAGS here for testing
#echo testing MSSA with flag $FLAGS

###########SET variables and options when testing using loadable so file invoked by opt
LLVMFLAGS="-mem2reg -rc -ander --debug-pass=Structure " #'-count-aa -dse -disable-opt -disable-inlining -disable-internalize'
LIBNAME=lib$TNAME

############don't need to touch here (please see run.sh script for meaning of the parameters)##########
if [[ $2 == 'opt' ]]
then
  $RUNSCRIPT $1 $TNAME "$LLVMFLAGS" $2
else
  $RUNSCRIPT $1 $TNAME "$FLAGS" $2
fi
