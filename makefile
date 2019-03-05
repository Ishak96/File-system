# name of output program
TARGET=fmanag

CC=gcc

# projects dirs and configuration files
BINDIR=bin
SRCDIR=src
INCDIR=include
TESTDIR=tests
OBJDIR=obj
DOXCONF=docsgen.conf

CFLAGS=-std=c99 -Wall -g -I$(INCDIR)

# getting source files and obj names
SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(INCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TESTS    := $(wildcard $(TESTDIR)/*.c)
TESTBINS := $(TESTS:$(TESTDIR)/%.c=$(BINDIR)/%)
TESTOBJ  := $(TESTS:$(TESTDIR)/%.c=$(OBJDIR)/%.o)

# flags to be used
LFLAGS=-lm

# make main executable
$(BINDIR)/$(TARGET): $(OBJECTS)
	$(CC) $^ $(LFLAGS) -o $@

# make obj files
$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# compile tests
test: $(TESTBINS)

$(TESTBINS): $(TESTOBJ) $(OBJECTS)
	$(CC) $(LFLAGS) $(filter-out obj/main.o $(filter-out $(patsubst $(BINDIR)/%,$(OBJDIR)/%.o,$@), $(TESTOBJ)), $(OBJECTS) $(TESTOBJ)) -o $@

$(TESTOBJ): $(TESTS)
	$(CC) $(CFLAGS) -c $(patsubst $(OBJDIR)/%.o,$(TESTDIR)/%.c,$@) -o $@


# generate docs
gendocs: $(SOURCES)
	doxygen $(DOXCONF)

.PHONY: clean
clean:
	rm -f obj/* bin/*
