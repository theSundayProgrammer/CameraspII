
INCLUDES = -I ../include
CXX_FLAGS = -std=c++14 -pthread -DRASPICAM_MOCK -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++

all : camerasp webserver
.PHONY : all

websrcs = controller.cpp\
	utils.cpp
webobjs = $(websrcs:%.cpp=$(BUILD_DIR)/%.o)
webdeps = $(websrcs:.cpp=$(BUILD_DIR)/.d)

srcs = frame_grabber.cpp \
	cam_still_base.cpp \
	cam_still.cpp \
	fopen.cpp \
	img_info.cpp \
	jpgconvert.cpp \
	raspicam_mock.cpp \
	timer.cpp \
	utils.cpp
objs = $(srcs:%.cpp=$(BUILD_DIR)/%.o)
deps = $(srcs:.cpp=$(BUILD_DIR)/.d)

camerasp: $(objs)
	$(CXX)   -o $@ $^ -pthread -Wunused -L/opt/vc/lib\
                   -lrt -lboost_filesystem -lboost_system\
                   -L../lib -ljpeg -ljson 

webserver: $(webobjs)
	$(CXX)   -o $@ $^ -pthread -Wunused -L/opt/vc/lib\
                   -lrt -lboost_filesystem -lboost_system\
                   -L../lib -ljpeg -ljson 

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -MP -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) $(deps) $(webdeps) camerasp webserver

-include $(deps)
-include $(webdeps)
