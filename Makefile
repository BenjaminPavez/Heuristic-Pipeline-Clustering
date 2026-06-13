CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O3

# Archivos fuente y objetos
SRCS = main.cpp utils.cpp evaluation.cpp constraints.cpp greedy.cpp tabu_search.cpp
OBJS = $(SRCS:.cpp=.o)

# Nombre del ejecutable
TARGET = dag_clusterer

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)