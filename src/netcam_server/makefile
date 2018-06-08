
INCLUDES = -I ../../include
CXX_FLAGS = -std=c++17 -Wno-psabi -g -pthread -O3 -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++-8.1.0
MKDIR_P = mkdir -p

all : directories netcam server 
.PHONY : all

directories: ${BUILD_DIR} 

srcs = main.cpp 
objs = $(srcs:%.cpp=$(BUILD_DIR)/%.o)
deps = $(srcs:.cpp=$(BUILD_DIR)/.d)

controller_srcs = controller.cpp 
controller_objs = $(controller_srcs:%.cpp=$(BUILD_DIR)/%.o)
controller_deps = $(controller_srcs:.cpp=$(BUILD_DIR)/.d)

${BUILD_DIR}:
	${MKDIR_P} ${BUILD_DIR}
	${MKDIR_P} ${BUILD_DIR}/../frame_grabber

netcam:  $(controller_objs)
	$(CXX)   -o $@ $^  -g -pthread -O3 -Wunused -L/opt/vc/lib\
                   -L../../lib -lcamera -ljson -lleveldb  \
                   -lrt -lboost_filesystem -lboost_system\
                   -lssl -lcrypto \
                   -lopencv_features2d \
                   -lopencv_objdetect \
                   -lopencv_imgproc \
                   -lopencv_core\
                   -ljpeg -lmmal -lmmal_core -lmmal_util

server: $(objs)
	$(CXX)   -o $@ $^ -pthread -O3 -Wunused -L/opt/vc/lib\
                   -L../../lib -lcamera -ljson -lleveldb  \
                   -lrt -lboost_filesystem -lboost_system\
                   -lssl -lcrypto \
                   -lopencv_features2d \
                   -lopencv_objdetect \
                   -lopencv_imgproc \
                   -lopencv_core\
                   -ljpeg -lmmal -lmmal_core -lmmal_util

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) $(deps) netcam 

-include $(deps)
