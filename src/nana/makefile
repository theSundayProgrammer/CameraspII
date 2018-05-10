CXX_FLAGS =  -pthread -O3 -DNANA_ENABLE_JPG -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++
MKDIR_P = mkdir -p

all : directories picture 
.PHONY : all

directories: ${BUILD_DIR} 

srcs = main.cpp 
objs = $(srcs:%.cpp=$(BUILD_DIR)/%.o)
deps = $(srcs:.cpp=$(BUILD_DIR)/.d)

${BUILD_DIR}:
	${MKDIR_P} ${BUILD_DIR}

picture : $(objs)
	$(CXX)   -o $@ $^ -O3 -Wunused \
		-lnana -lboost_filesystem -lboost_system\
		-lrt -lX11 -pthread -lXft -lfontconfig \
    -ljpeg 

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) $(deps) picture 

-include $(deps)
#g++ -o picture main.cpp -DNANA_ENABLE_JPG   -lnana -lboost_filesystem -lboost_system -lrt -lX11 -pthread -lXft -lfontconfig -ljpeg
