# The MIDASSYS should be defined prior the use of this Makefile
ifndef MIDASSYS
missmidas::
	@echo "...";
	@echo "Missing definition of environment variable 'MIDASSYS' !";
	@echo "...";
endif

OS_DIR = linux
OSFLAGS = -DOS_LINUX -Dextname
CFLAGS = -O3 -Wall
LIBS = -lm -lz -lutil -lnsl -lpthread -lrt -lusb-1.0

INC_DIR   = $(MIDASSYS)/include
LIB_DIR   = $(MIDASSYS)/$(OS_DIR)/lib
SRC_DIR   = $(MIDASSYS)/src

UFE = dfe

####################################################################
# Lines below here should not be edited
####################################################################

# MIDAS library
LIB = $(LIB_DIR)/libmidas.a

# compiler
CC = gcc
CXX = g++
CFLAGS += -g -I$(INC_DIR)
LDFLAGS +=

all: $(UFE)

$(UFE): $(LIB) $(LIB_DIR)/mfe.o $(UFE).o
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $(UFE) $(UFE).o $(LIB_DIR)/mfe.o $(LIB) $(LIBS)
	strip $(UFE)

%.o: %.c
	$(CXX) $(USERFLAGS) $(CFLAGS) $(OSFLAGS) -o $@ -c $<

clean::
	rm -f *.o *~ \#* $(UFE)
