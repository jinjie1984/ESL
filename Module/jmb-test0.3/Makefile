SOURCE :=		./*.c\

INCLUDE :=

LIBS :=	

OBJS := $(patsubst %.c,%.o,$(wildcard $(SOURCE)))

TARGET := jmb-test

CC := gcc
AS := nasm

LDFLAGS :=
DEFINES := 

CFLAGS := -g -Wall -m64 -O0 $(DEFINES) $(INCLUDE) 
CXXFLAGS := $(CFLAGS) 

.PHONY : everything objs clean veryclean rebuild
everything : $(TARGET)
all : $(TARGET)
objs : $(OBJS)
rebuild : veryclean everything

clean: 
	rm -rf $(OBJS)
	rm -rf $(TARGET)

$(TARGET) : $(OBJS) routines-x86-64bit.asm
	$(AS) -f elf64 routines-x86-64bit.asm -o routines-x86-64bit.o
	$(CC)  $(CXXFLAGS) -o $@ $(OBJS) routines-x86-64bit.o $(LDFLAGS) $(LIBS)

