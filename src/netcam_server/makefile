
INCLUDES = -I ../../include
CXX_FLAGS = -std=c++14 -pthread -O3 -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++-6
MKDIR_P = mkdir -p

all : directories netcam server 
.PHONY : all

directories: ${BUILD_DIR} 

srcs = main.cpp \
	cam_still_base.cpp \
	cam_still.cpp \
	img_info.cpp \
	jpgconvert.cpp \
	timer.cpp \
	utils.cpp
objs = $(srcs:%.cpp=$(BUILD_DIR)/%.o)
deps = $(srcs:.cpp=$(BUILD_DIR)/.d)

controller_srcs = controller.cpp utils.cpp
controller_objs = $(controller_srcs:%.cpp=$(BUILD_DIR)/%.o)
controller_deps = $(controller_srcs:.cpp=$(BUILD_DIR)/.d)

${BUILD_DIR}:
	${MKDIR_P} ${BUILD_DIR}

netcam:  $(controller_objs)
	$(CXX)   -o $@ $^ $(CXX_FLAGS) $(INCLUDES) -Wunused -L/opt/vc/lib\
                   -L../../lib -ljson \
                   -lrt -lboost_filesystem -lboost_system

server: $(objs)
	$(CXX)   -o $@ $^ -pthread -O3 -Wunused -L/opt/vc/lib\
                   -L../../lib -ljson -lleveldb\
                   -lrt -lboost_filesystem -lboost_system\
                   -ljpeg -lmmal -lmmal_core -lmmal_util

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) $(deps) netcam 

-include $(deps)
