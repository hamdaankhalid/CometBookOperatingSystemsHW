CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

TARGET = cfs 
SRCS = cfs.cpp

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)

lint:
	clang-format -i cfs.cpp
