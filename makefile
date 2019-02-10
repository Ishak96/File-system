# name of output program
TARGET=fmanag

CC=gcc

# projects dirs and configuration files
BINDIR=bin
SRCDIR=src
INCDIR=include
OBJDIR=obj
DOXCONF=docsgen.conf

CFLAGS=-std=c99 -Wall -I$(INCDIR)

# getting source files and obj names
SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(INCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# flags to be used
LFLAGS=-lm

# make main executable
$(BINDIR)/$(TARGET): $(OBJECTS)
	$(CC) $^ $(LFLAGS) -o $@

# make obj files
$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# generate docs
gendocs: $(SOURCES)
	doxygen $(DOXCONF)

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(TARGET)