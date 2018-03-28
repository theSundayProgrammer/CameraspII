
INCLUDES = -I ../include
CXX_FLAGS = -std=c++14 -pthread -O3 -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++

all : pinger
.PHONY : all

srcs = pinger.cpp 
objs = $(srcs:%.cpp=$(BUILD_DIR)/%.o)
deps = $(srcs:.cpp=$(BUILD_DIR)/.d)

pinger: $(objs)
	$(CXX)   -o $@ $^ -pthread -O3 -Wunused 
$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) $(deps)  pinger

-include $(deps)
