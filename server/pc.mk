
INCLUDES = -I ../include
CXX_FLAGS = -std=c++14 -pthread -DRASPICAM_MOCK -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++

all : camerasp
.PHONY : all

http:
	$(MAKE) -C $@

srcs = $(wildcard *.cpp)
objs = $(srcs:%.cpp=$(BUILD_DIR)/%.o)
deps = $(srcs:.cpp=$(BUILD_DIR)/.d)

camerasp: $(objs)
	$(CXX)   -o $@ $^ -pthread -L/opt/vc/lib -L../lib -ljpeg -lvia-http -ljson 

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -MP -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) $(deps) camerasp

-include $(deps)
