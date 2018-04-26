INCLUDES = -I ../../include
CXX_FLAGS = -pthread -std=c++14
BUILD_DIR=./build
LIB_DIR=../../lib
CXX=g++-6

MKDIR_P = mkdir -p

.PHONY: directories

all: directories ${LIB_DIR}/libjson.a

directories: ${BUILD_DIR} ${LIB_DIR}

${LIB_DIR}:
	${MKDIR_P} ${LIB_DIR}

${BUILD_DIR}:
	${MKDIR_P} ${BUILD_DIR}

srcs = $(wildcard *.cpp)
objs = $(srcs:%.cpp=$(BUILD_DIR)/%.o)

${LIB_DIR}/libjson.a: $(objs)
	ar rcs $@ $^

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES)   -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) 

