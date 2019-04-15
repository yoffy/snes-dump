CXXFLAGS=-g -Os -Wall -Wextra
TARGET=snes-dump
OBJS=snes-dump.o

all: $(TARGET)

clean:
	$(RM) $(TARGET) $(OBJS)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)
