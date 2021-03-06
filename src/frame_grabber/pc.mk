
INCLUDES = -I ../include
CXX_FLAGS = -std=c++14 -pthread -O3 -DRASPICAM_MOCK -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++

all : camerasp webserver pinger
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

cli_srcs = pinger.cpp 
cli_objs = $(cli_srcs:%.cpp=$(BUILD_DIR)/%.o)
cli_deps = $(cli_srcs:.cpp=$(BUILD_DIR)/.d)

pinger: $(cli_objs)
	$(CXX)   -o $@ $^ -pthread -O3 -Wunused 
camerasp: $(objs)
	$(CXX)   -o $@ $^ -pthread -O3 -Wunused -L/opt/vc/lib\
                   -lrt -lboost_filesystem -lboost_system\
                   -L../lib -ljson -ljpeg 
webserver: $(webobjs)
	$(CXX)   -o $@ $^ -pthread -O3 -Wunused -L/opt/vc/lib\
                   -lrt -lboost_filesystem -lboost_system\
                   -L../lib -ljson -ljpeg 

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) $(cli_objs) $(webobjs) $(deps) $(webdeps) $(cli_deps) camerasp webserver pinger

-include $(webdeps)
-include $(deps)
-include $(cli_deps)
