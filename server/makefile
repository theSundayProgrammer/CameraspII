
INCLUDES = -I ../include
CXX_FLAGS = -std=c++14 -pthread -O3 -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++-6

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
	file_saver.cpp \
        mot_detect.cpp \
        smtp_client.cpp \
	utils.cpp
objs = $(srcs:%.cpp=$(BUILD_DIR)/%.o)
deps = $(srcs:.cpp=$(BUILD_DIR)/.d)

camerasp: $(objs)
	$(CXX)   -o $@ $^ -pthread -O3 -Wunused -L/opt/vc/lib\
                   -L../lib -ljson \
                   -lrt -lboost_filesystem -lboost_system\
                   -lssl -lcrypto \
                   -lopencv_features2d \
                   -lopencv_highgui \
                   -lopencv_videoio \
                   -lopencv_imgcodecs \
                   -lopencv_objdetect \
                   -lopencv_imgproc \
                   -lopencv_core\
                   -ljpeg -lmmal -lmmal_core -lmmal_util
	cp $@ /home/pi/bin
webserver: $(webobjs)
	$(CXX)   -o $@ $^ -pthread -O3 -Wunused -L/opt/vc/lib\
                   -lrt -lboost_filesystem -lboost_system\
                   -L../lib -ljson -ljpeg 
	cp $@ /home/pi/bin
$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) $(webobjs) $(deps) $(webdeps)  camerasp webserver

-include $(webdeps)
-include $(deps)
