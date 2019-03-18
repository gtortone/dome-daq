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

DRIVERS         = multibit.o multi.o ch-freq.o ch-enable.o ch-power.o ch-ocurr.o

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

dfe:  $(LIB) $(LIB_DIR)/mfe.o dfe.o runcontrol.o $(DRIVERS)
	$(CC) -o dfe dfe.o $(LIB_DIR)/mfe.o runcontrol.o $(DRIVERS) $(LIB) $(LDFLAGS) $(LIBS)
	strip dfe

dfe.o:	dfe.cpp
	$(CXX) $(CFLAGS) $(OSFLAGS) -c $< -o $@

runcontrol.o: runcontrol.cpp
	$(CXX) $(CFLAGS) $(OSFLAGS) -c $< -o $@

multi.o: multi.c
	$(CC) $(CFLAGS) $(OSFLAGS) -c $< -o $@

multibit.o: multibit.c
	$(CC) $(CFLAGS) $(OSFLAGS) -c $< -o $@

ch-freq.o: ch-freq.cpp
	$(CXX) $(CFLAGS) $(OSFLAGS) -c $< -o $@

ch-enable.o: ch-enable.cpp
	$(CXX) $(CFLAGS) $(OSFLAGS) -c $< -o $@

ch-power.o: ch-power.cpp
	$(CXX) $(CFLAGS) $(OSFLAGS) -c $< -o $@

ch-ocurr.o: ch-ocurr.cpp
	$(CXX) $(CFLAGS) $(OSFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) $(OSFLAGS) -c $<

clean:
	rm -f *.o *~ \#* dfe

