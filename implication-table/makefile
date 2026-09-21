COMPILER = g++
FLAGS    = -Wall -std=c++17 -g -MMD -MP -finput-charset=UTF-8 -fexec-charset=UTF-8
TARGET   = main.exe
SRCS     = $(wildcard src/*.cpp)
OBJS     = $(SRCS:src/%.cpp=build/%.o)
DEPS     = $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(COMPILER) $(FLAGS) -o $@ $^ -lwinmm

build/%.o: src/%.cpp | build
	$(COMPILER) -Iinclude $(FLAGS) -c -o $@ $<

build:
	mkdir build

-include $(DEPS)

run: $(TARGET)
	powershell -NoProfile -Command "Start-Process pwsh -ArgumentList '-Command','.\$(TARGET)' -WindowStyle Maximized"

clean:
	del /f $(TARGET) build\*.o build\*.d