
INCLUDES = -I ../include
CXX_FLAGS = -std=c++14 -pthread -g -O3 -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++

all : test
.PHONY : all


srcs = main.cpp smtp_client.cpp
objs = $(srcs:%.cpp=$(BUILD_DIR)/%.o)
deps = $(srcs:.cpp=$(BUILD_DIR)/.d)



test: $(objs)
	$(CXX) -g  -o $@ $^ -pthread -O3 -Wunused -L/opt/vc/lib\
                   -lcurl 
$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) $(deps) test


-include $(deps)
