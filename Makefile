CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O3

SRCS = main.cpp utils.cpp evaluation.cpp constraints.cpp greedy.cpp tabu_search.cpp reporting.cpp
OBJS = $(SRCS:.cpp=.o)

TARGET = dag_clusterer

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET) $(FILE)