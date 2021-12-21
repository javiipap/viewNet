PROGNAME   = viewNet
CC       = g++
CFLAGS   = -std=c++2a -Wall
LINKER   = g++

SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin
TESTDIR  = tests

SRC  := $(patsubst $(SRCDIR)/%.cc, $(OBJDIR)/%.o, $(wildcard $(SRCDIR)/*.cc))
LIB_SRC  := $(patsubst $(SRCDIR)/%.cc, $(OBJDIR)/%.o, $(filter-out $(SRCDIR)/main.cc, $(wildcard $(SRCDIR)/*.cc)))
TESTS_SRC := $(patsubst $(TESTDIR)/%.cc, $(TESTDIR)/%.o, $(wildcard $(TESTDIR)/*.cc))

$(PROGNAME): $(SRC)
	@mkdir -p $(BINDIR)
	@echo $(SRC)
	@$(LINKER) $(CFLAGS) $(SRC) -o $(BINDIR)/$(PROGNAME)
	@echo "Linking complete!"

$(OBJDIR)/%.o: $(SRCDIR)/%.cc
	@mkdir -p $(OBJDIR)
	@echo $< $@
	@$(CC) -std=c++2a -g -c $< -o $@

tests: $(TESTS_SRC)
	@echo Compiling tests
	@echo $(TESTS_SRC)

$(TESTDIR)/%.o: $(LIB_SRC)
	@mkdir -p $(BINDIR)/tests
	@$(CC) $(CFLAGS) -g -c $(patsubst $(TESTDIR)/%.o, $(TESTDIR)/%.cc, $@) -o $(patsubst $(TESTDIR)/%.o, $(OBJDIR)/%.o, $@)
	@$(LINKER) $(CFLAGS) -o $(patsubst $(TESTDIR)/%.o, $(BINDIR)/tests/%, $@) $(patsubst $(TESTDIR)/%.o, $(OBJDIR)/%.o, $@) $(LIB_SRC)

clean:
	rm -rf $(OBJDIR)/*
