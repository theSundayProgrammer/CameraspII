
INCLUDES = -I ../include
CXX_FLAGS = -std=c++14 -pthread -g -O3 -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++
LDFLAGS=-lcrypto -lssl -lboost_system -lrt -lboost_filesystem

all : test
.PHONY : all



objs = $(BUILD_DIR)/main.o 
deps = $(BUILD_DIR)/main.d 

test: $(objs)
	$(CXX) -g  -o $@ $^ -pthread -O3 -Wunused -L/opt/vc/lib $(LDFLAGS)

$(BUILD_DIR)/main.o: main.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

$(BUILD_DIR)/smtp_client.o: ../server/smtp_client.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) $(deps) test


-include $(deps)
