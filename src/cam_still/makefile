
INCLUDES = -I ../../include
CXX_FLAGS = -std=c++17 -Wno-psabi -pthread -O3 -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++-8.1.0
MKDIR_P = mkdir -p
LIB_DIR=../../lib

all : directories ${LIB_DIR}/libcamera.a
.PHONY : all

directories: ${BUILD_DIR} 

srcs = cam_still_base.cpp \
	cam_still.cpp \
	fopen.cpp \
	img_info.cpp \
	jpgconvert.cpp \
	timer.cpp \
	save_ldb.cpp \
	level_db.cpp \
	frame_grabber.cpp \
	client_msg_queues.cpp\
	mot_detect.cpp \
	smtp_client.cpp \
	utils.cpp
objs = $(srcs:%.cpp=$(BUILD_DIR)/%.o)
deps = $(srcs:.cpp=$(BUILD_DIR)/.d)

${BUILD_DIR}:
	${MKDIR_P} ${BUILD_DIR}

${LIB_DIR}/libcamera.a: $(objs)
	ar rcs $@ $^

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) $(deps) camerasp 

-include $(deps)
