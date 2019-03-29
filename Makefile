ifndef MIDASSYS
missmidas::
	@echo "...";
	@echo "Missing definition of environment variable 'MIDASSYS' !";
	@echo "...";
endif

OS_DIR = linux
LIBS = -lm -lutil -lpthread -lrt -lusb-1.0
OSFLAGS = -DOS_LINUX

INC_DIR    = $(MIDASSYS)/include
LIB_DIR    = $(MIDASSYS)/$(OS_DIR)/lib
MXML_DIR   = $(MIDASSYS)/../mxml

####################################################################
# Lines below here should not be edited
####################################################################

LIB = $(LIB_DIR)/libmidas.a

# compiler
CC = cc
CXX = g++
CFLAGS = -O2 -Wall -I$(INC_DIR) 
LDFLAGS =

all: dfe

dfe:  $(LIB) $(LIB_DIR)/mfe.o dfe.o runcontrol.o 
	$(CC) -o dfe dfe.o $(LIB_DIR)/mfe.o runcontrol.o $(LIB) $(LDFLAGS) $(LIBS)
	strip dfe

dfe.o:	dfe.cpp
	$(CXX) $(CFLAGS) $(OSFLAGS) -c $< -o $@

runcontrol.o: runcontrol.cpp
	$(CXX) $(CFLAGS) $(OSFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) $(OSFLAGS) -c $<

clean:
	rm -f *.o *~ \#* dfe

