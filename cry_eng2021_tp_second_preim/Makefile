# the compiler: gcc for C program, define as g++ for C++
CC = gcc
CFLAGS  = -Wall -O3 -march=native
# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
ifdef test
    CFLAGS  +=  -DRUNTESTS=1 
endif

ifdef debug
    CFLAGS  += -g
endif

SOURCES := $(wildcard *.c)
OBJECTS := $(patsubst %.c, %.o, $(SOURCES))
# the build target executable:
TESTPROGRAM = second_preim_48_fillme

all: $(TESTPROGRAM)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf *.o $(TESTPROGRAM)
