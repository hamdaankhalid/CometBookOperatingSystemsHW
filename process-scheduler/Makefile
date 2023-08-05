CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

TARGET = cfs 
SRCS = cfs.cpp

ANALYZER_TARGET = analyze
ANALYZER_SRC = analyze.cpp

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

analyzer:
	$(CXX) $(CXXFLAGS) -o $(ANALYZER_TARGET) $(ANALYZER_SRC)

clean:
	rm -f $(TARGET) && rm -f $(ANALYZER_TARGET)

lint:
	clang-format -i cfs.cpp analyze.cpp
