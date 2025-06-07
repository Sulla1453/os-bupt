CXX = g++
CXXFLAGS = -std=c++17 -Wall -g
SRC = main.cpp Process/Process.cpp Process/ProcessManager.cpp Semaphore.cpp ResourceManager.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = os_sim

all: $(TARGET)

$(TARGET): $(OBJ)
    $(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
    $(CXX) $(CXXFLAGS) -c $< -o $@

clean:
    rm -f $(OBJ) $(TARGET)

.PHONY: all clean
# 没有装make工具就用以下命令行
# g++ -std=c++17 main.cpp Process/Process.cpp Process/ProcessManager.cpp Process/Semaphore.h Process/Semaphore.cpp Process/ResourceManager.h Process/ResourceManager.cpp -o os_sim
