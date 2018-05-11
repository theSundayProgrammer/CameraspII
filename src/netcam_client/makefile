
INCLUDES = -I ../../include
CXX_FLAGS = -std=c++14 -pthread -O3 -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++
MKDIR_P = mkdir -p

all : directories netcam 
.PHONY : all

directories: ${BUILD_DIR} 

srcs = main.cpp 
objs = $(srcs:%.cpp=$(BUILD_DIR)/%.o)
deps = $(srcs:.cpp=$(BUILD_DIR)/.d)

${BUILD_DIR}:
	${MKDIR_P} ${BUILD_DIR}

netcam: $(objs)
	$(CXX)   -o $@ $^ -lrt -pthread -O3 -Wunused 

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) $(deps) netcam 

-include $(deps)
