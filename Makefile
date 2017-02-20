#Makefile for simple programs
##################################################
PRGM = mySpider
CC = gcc  
CPPFLAGS =  
CFLAGS = -Wall -g `pkg-config --cflags glib-2.0`  
CCOMPILE = $(CC) $(CPPFLAGS) $(CFLAGS) -c  
CXX = g++  
CXXFLAGS = -Wall -g `pkg-config --cflags glib-2.0`  
CXXCOMPILE = $(CXX) $(CXXFLAGS) -c  
LDFLAGS = `pkg-config --libs glib-2.0` -lpthread -levent -lgumbo 
LINKCXX = $(LDFLAGS)  
CSRCS := $(wildcard *.)  
COBJS := $(patsubst %.c,%.o,$(CSRCS))  
CXXSRCS := $(wildcard *.cpp)  
CXXOBJS := $(patsubst %.cpp,%.o,$(CXXSRCS))  
OBJS = $(COBJS) $(CXXOBJS)  
$(PRGM):$(OBJS)  
	$(CXX) -o $(PRGM) $(OBJS) $(LINKCXX)
%.o:%.c  
	$(CCOMPILE) $< -o $@  
%o:%.cpp  
	$(CXXCOMPILE) $< -o $@  
.PHONY:clean  
clean:  
	rm -f $(OBJS) $(PRGM) $(APPLIBSO) core* tmp*
