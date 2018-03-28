
INCLUDES = -I ../include
CXX_FLAGS = -std=c++14 -pthread -g -O3 -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++

all : test
.PHONY : all



objs = $(BUILD_DIR)/main.o $(BUILD_DIR)/smtp_client.o
deps = $(BUILD_DIR)/main.d $(BUILD_DIR)/smtp_client.d

test: $(objs)
	$(CXX) -g  -o $@ $^ -pthread -O3 -Wunused -L/opt/vc/lib\
                   -lcurl 

$(BUILD_DIR)/main.o: main.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

$(BUILD_DIR)/smtp_client.o: ../server/smtp_client.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) $(deps) test


-include $(deps)
