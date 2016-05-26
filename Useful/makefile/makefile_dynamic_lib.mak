######################################
#
# Generic makefile
#
# by JieJin
# email: jiejin@zhaoxin.com
#
# Copyright (c) 2016 ZhaoXin
# All rights reserved.
# 
# No warranty, no liability;
# you use this at your own risk.
#
# You are free to modify and
# distribute this without giving
# credit to the original author.
#
######################################

SYSTEMC_HOME := /esl_tools/systemc-2.3.1

#source files, only support .cpp for this makefile
SOURCE  := 		./src/jsc_cmd/*.cpp\
				./src/jsc_misc/*.cpp\
				./src/jsc_tlm/*.cpp\

#include path, indicate the header file directions
INCLUDE := 		-I$(SYSTEMC_HOME)/include\
				-I./include\

#Library name and path
LIBS    := 		-lsystemc\
				-L$(SYSTEMC_HOME)/lib-linux64
				
#target you can change test to what you want
TARGET  := ./lib/libesl_utils.so

#objective file
OBJS := $(patsubst %.cpp,%.o,$(wildcard $(SOURCE)))
 
#compile and lib parameter
CC      := g++

LDFLAGS:= 
DEFINES:=

CFLAGS  := -g -Wall -m64 -O3 -fPIC  $(DEFINES) $(INCLUDE)
CXXFLAGS:= $(CFLAGS)

SHARE := -shared -o
 
#Nothing need to be modified
.PHONY : everything objs clean veryclean rebuild
 
everything : $(TARGET)
 
all : $(TARGET)
 
objs : $(OBJS)
 
rebuild: veryclean everything
               
clean :
	rm -fr $(OBJS)
   
veryclean : clean
	rm -fr $(TARGET)
 
$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) $(SHARE) $@ $(OBJS) $(LDFLAGS) $(LIBS)
