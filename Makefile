# ------------------------------------------------
# Generic Makefile
#
# Author: yanick.rochon@gmail.com
# Date  : 2011-08-10
#
# Changelog :
#   2010-11-05 - first version
#   2011-08-10 - added structure : sources, objects, binaries
#                thanks to http://stackoverflow.com/users/128940/beta
#   2017-04-24 - changed order of linker params
# ------------------------------------------------

# project name (generate executable with this name)
PROGNAME   = viewNet

CC       = g++
# compiling flags here
CFLAGS   = -std=c++11 -Wall

LINKER   = g++
# linking flags here
LFLAGS   = -Wall

# change these to proper directories where each file should be
SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

SRC  := $(patsubst $(SRCDIR)/%.cc, $(BINDIR)/%.o, $(wildcard $(SRCDIR)/*.cc))

$(PROGNAME): $(SRC)
	@echo $(SRC)
	@$(LINKER) $(SRC) $(LFLAGS) -o $(BINDIR)/$(PROGNAME)
	@echo "Linking complete!"

$(BINDIR)/%.o: $(SRCDIR)/%.cc
	@echo $< $@
	@$(CC) -g -c $< -o $@

clean:
	rm -rf $(BINDIR)/*