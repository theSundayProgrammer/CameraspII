
INCLUDES = -I ../../include
CXX_FLAGS = -std=c++14 -pthread -O3 -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++
MKDIR_P = mkdir -p

all : directories netcam server 
.PHONY : all

directories: ${BUILD_DIR} 

srcs = main.cpp \
	../frame_grabber/cam_still_base.cpp \
	../frame_grabber/cam_still.cpp \
	../frame_grabber/img_info.cpp \
	../frame_grabber/jpgconvert.cpp \
	timer.cpp \
	../frame_grabber/utils.cpp
objs = $(srcs:%.cpp=$(BUILD_DIR)/%.o)
deps = $(srcs:.cpp=$(BUILD_DIR)/.d)

controller_srcs = controller.cpp ../frame_grabber/utils.cpp
controller_objs = $(controller_srcs:%.cpp=$(BUILD_DIR)/%.o)
controller_deps = $(controller_srcs:.cpp=$(BUILD_DIR)/.d)

${BUILD_DIR}:
	${MKDIR_P} ${BUILD_DIR}
	${MKDIR_P} ${BUILD_DIR}/../frame_grabber

netcam:  $(controller_objs)
	$(CXX)   -o $@ $^ -pthread -O3 -Wunused -L/opt/vc/lib\
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
