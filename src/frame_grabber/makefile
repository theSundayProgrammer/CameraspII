
INCLUDES = -I ../../include
CXX_FLAGS = -std=c++14 -pthread -O3 -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++-6
MKDIR_P = mkdir -p

all : directories camerasp 
.PHONY : all

directories: ${BUILD_DIR} 

srcs = main.cpp \
	cam_still_base.cpp \
	cam_still.cpp \
	fopen.cpp \
	img_info.cpp \
	jpgconvert.cpp \
	raspicam_mock.cpp \
	timer.cpp \
	file_saver.cpp \
        mot_detect.cpp \
	utils.cpp
objs = $(srcs:%.cpp=$(BUILD_DIR)/%.o)
deps = $(srcs:.cpp=$(BUILD_DIR)/.d)

${BUILD_DIR}:
	${MKDIR_P} ${BUILD_DIR}

camerasp: $(objs)
	$(CXX)   -o $@ $^ -pthread -O3 -Wunused -L/opt/vc/lib\
                   -L../../lib -ljson -lleveldb\
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
	cp cacert.pem /home/pi/bin

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) $(deps) camerasp 

-include $(deps)
