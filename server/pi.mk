
INCLUDES = -I ../include
CXX_FLAGS = -std=c++14 -pthread -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++-6

all : camerasp
.PHONY : all

http:
	$(MAKE) -C $@

srcs = $(wildcard *.cpp)
objs = $(srcs:%.cpp=$(BUILD_DIR)/%.o)
deps = $(srcs:.cpp=$(BUILD_DIR)/.d)

camerasp: $(objs)
	$(CXX)   -o $@ $^ -pthread -L/opt/vc/lib \
	-lboost_filesystem -lboost_system -ljpeg \
	-L../lib -ljson \
	-lmmal -lmmal_core -lmmal_util

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -MP -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) $(deps) camerasp

-include $(deps)
