
INCLUDES = -I ../include
CXX_FLAGS = -std=c++14 -pthread -O3 -DASIO_STANDALONE 
BUILD_DIR=./build
CXX=g++

all : ping_server ping_client
.PHONY : all

srv_srcs = server.cpp
srv_objs = $(srv_srcs:%.cpp=$(BUILD_DIR)/%.o)
srv_deps = $(srv_srcs:.cpp=$(BUILD_DIR)/.d)

cli_srcs = client.cpp 
cli_objs = $(cli_srcs:%.cpp=$(BUILD_DIR)/%.o)
cli_deps = $(cli_srcs:.cpp=$(BUILD_DIR)/.d)


ping_server: $(srv_objs)
	$(CXX)   -o $@ $^ -pthread -O3 -Wunused -L/opt/vc/lib\
                   -lrt -lboost_filesystem -lboost_system\
                   -L../lib -ljson -ljpeg 
ping_client: $(cli_objs)
	$(CXX)   -o $@ $^ -pthread -O3 -Wunused -L/opt/vc/lib\
                   -lrt -lboost_filesystem -lboost_system\
                   -L../lib -ljson -ljpeg 

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(srv_objs) $(cli_objs) $(srv_deps) $(cli_deps) camerasp webserver

-include $(cli_deps)
-include $(srv_eps)
