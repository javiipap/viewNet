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
SERVER   = server
CLIENT   = client
SERVER_THREADS   = threaded_server
CLIENT_THREADS   = threaded_client

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

SERVER_SRC  := $(patsubst $(SRCDIR)/%.cc, $(BINDIR)/%.o, $(filter-out $(SRCDIR)/client.cc $(SRCDIR)/client_threads.cc $(SRCDIR)/server_threads.cc, $(wildcard $(SRCDIR)/*.cc)))
CLIENT_SRC  := $(patsubst $(SRCDIR)/%.cc, $(BINDIR)/%.o, $(filter-out $(SRCDIR)/server.cc $(SRCDIR)/client_threads.cc $(SRCDIR)/server_threads.cc, $(wildcard $(SRCDIR)/*.cc)))

SERVER_THREADS_SRC  := $(filter-out $(SRCDIR)/server.cc, $(SERVER_SRC))
CLIENT_THREADS_SRC  := $(filter-out $(SRCDIR)/client.cc, $(CLIENT_SRC))


$(SERVER): $(SERVER_SRC)
	@echo $(SERVER_SRC)
	@$(LINKER) $(SERVER_SRC) $(LFLAGS) -o $(BINDIR)/$@
	@echo "Linking complete!"

$(CLIENT): $(CLIENT_SRC)
	@echo $(CLIENT_SRC)
	@$(LINKER) $(CLIENT_SRC) $(LFLAGS) -o $(BINDIR)/$@
	@echo "Linking complete!"

$(SERVER_THREADS): $(SERVER_THREADS_SRC)
	@echo $(SERVER_THREADS_SRC)
	@$(LINKER) $(SERVER_THREADS_SRC) $(LFLAGS) -o $(BINDIR)/$@
	@echo "Linking complete!"

$(CLIENT_THREADS): $(CLIENT_THREADS_SRC)
	@echo $(CLIENT_THREADS_SRC)
	@$(LINKER) $(CLIENT_THREADS_SRC) $(LFLAGS) -o $(BINDIR)/$@
	@echo "Linking complete!"

$(BINDIR)/%.o: $(SRCDIR)/%.cc
	@echo $< $@
	@$(CC) -g -c $< -o $@

clean:
	rm -rf $(BINDIR)/*