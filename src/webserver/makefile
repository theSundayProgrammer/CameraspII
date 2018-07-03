
INCLUDES = -I ../../include
CXX_FLAGS = -std=c++17  -Wno-psabi -pthread -O3 -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++-8.1.0
MKDIR_P = mkdir -p

all : directories webserver
.PHONY : all

directories: ${BUILD_DIR} 



${BUILD_DIR}:
	${MKDIR_P} ${BUILD_DIR}

webserver: ${BUILD_DIR}/controller.o ${BUILD_DIR}/server_msg_queues.o
	$(CXX)   -o $@ $^ -pthread -O3 -Wunused -L/opt/vc/lib\
                   -lrt -lboost_filesystem -lboost_system\
                   -L../../lib -lcamera -ljson -ljpeg 
	cp $@ /home/pi/bin


$(BUILD_DIR)/controller.o: controller.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@
$(BUILD_DIR)/server_msg_queues.o: server_msg_queues.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@
.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM)  webserver
	$(RM)  -r ${BUILD_DIR}

-include $(webdeps)
