##===- projects/sample/tools/sample/Makefile ---------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../../..

#
# Give the name of the tool.
#
TOOLNAME=rc

#
# List libraries that we'll need
# We use LIBS because sample is a dynamic library. a
# !!Should always consider the dependence of each library, the parent library should place at the end of the line
USEDLIBS = rc.a wpa.a mssa.a 

LINK_COMPONENTS := bitreader bitwriter asmparser irreader scalaropts ipo codegen 

#LINK_COMPONENTS = all

#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

