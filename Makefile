EXECUTABLE      := gossip.c
OUTPUT := p4

CXX        := g++
CC         := gcc
LINK       := g++ -fPIC

INCLUDES  += -I. -I/ncsu/gcc346/include/c++/ -I/ncsu/gcc346/include/c++/3.4.6/backward 
LIB       := -L/ncsu/gcc346/lib

default:
	$(CC) -v -g $(EXECUTABLE) -o $(OUTPUT) $(INCLUDES) $(LIB) -lpthread 

clean:
	rm -f $(OUTPUT)

