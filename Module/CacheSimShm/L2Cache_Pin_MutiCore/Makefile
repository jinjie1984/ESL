SOURCE :=		./*.cpp\

INCLUDE :=  -I./\

LIBS :=	

OBJS := $(patsubst %.cpp,%.o,$(wildcard $(SOURCE)))

TARGET := simulation

CC := g++

LDFLAGS :=
DEFINES := 

CFLAGS := -g -Wall -m64 -O3 -pthread $(DEFINES) $(INCLUDE) 
CXXFLAGS := $(CFLAGS) 

.PHONY : everything objs clean veryclean rebuild
everything : $(TARGET)
all : $(TARGET)
objs : $(OBJS)
rebuild : veryclean everything

clean: 
	rm -rf $(OBJS)
	rm -rf $(TARGET)

$(TARGET) : $(OBJS)
	$(CC)  $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

