SOURCE :=		./*.c\

INCLUDE :=

LIBS :=	

OBJS := $(patsubst %.c,%.o,$(wildcard $(SOURCE)))

TARGET := simulation

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

$(TARGET) : $(OBJS) test.asm
	$(AS) -f elf64 test.asm -o test.o
	$(CC)  $(CXXFLAGS) -o $@ $(OBJS) test.o $(LDFLAGS) $(LIBS)

