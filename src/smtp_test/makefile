
INCLUDES = -I ../include
CXX_FLAGS = -pthread -g -DASIO_STANDALONE -Wtrigraphs
BUILD_DIR=./build
CXX=g++-6
LDFLAGS=-lcrypto -lssl -lboost_system  -lboost_filesystem

all : test
.PHONY : all



objs = $(BUILD_DIR)/main.o $(BUILD_DIR)/smtp_mail.o $(BUILD_DIR)/utils.o
deps = $(BUILD_DIR)/main.d $(BUILD_DIR)/smtp_mail.d $(BUILD_DIR)/utils.d

test: $(objs)
	$(CXX) -g  -o $@ $^ -pthread -O3 -Wunused -L/opt/vc/lib $(LDFLAGS)

$(BUILD_DIR)/main.o: main.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

$(BUILD_DIR)/utils.o: utils.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

$(BUILD_DIR)/smtp_mail.o: ../server/smtp_mail.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@

.PHONY: clean

# $(RM) is rm -f by default
clean:
	$(RM) $(objs) $(deps) test


-include $(deps)
