INCLUDES = -I ../include
CXX_FLAGS = -pthread -std=c++14
BUILD_DIR=./build
CXX=g++-6


srcs = $(wildcard *.cpp)
objs = $(srcs:%.cpp=$(BUILD_DIR)/%.o)

../lib/libjson.a: $(objs)
	ar rcs $@ $^

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES)   -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) 

