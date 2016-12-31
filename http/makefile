INCLUDES = -I ../include
CXX_FLAGS = -std=c++14 -pthread -DASIO_STANDALONE -Wno-trigraphs
BUILD_DIR=./build
CXX=g++


srcs = $(wildcard *.cpp)
objs = $(srcs:%.cpp=$(BUILD_DIR)/%.o)

../lib/libvia-http.a: $(objs)
	ar rcs $@ $^

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES)   -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) 

