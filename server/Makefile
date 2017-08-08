
BUILD_DIR=./build
OBJS=$(BUILD_DIR)/RaspiCamControl.o $(BUILD_DIR)/RaspiCLI.o $(BUILD_DIR)/RaspiPreview.o $(BUILD_DIR)/RaspiStill.o \
      $(BUILD_DIR)/mot_detect.o $(BUILD_DIR)/smtpClient.o
LDFLAGS+=-lmmal -lmmal_core -lmmal_util
INCLUDES+=-I$(HOME)/projects/userland -I../include
STARTER_OBJS=$(BUILD_DIR)/RaspiStillStart.o $(BUILD_DIR)/utils.o

CFLAGS+= -std=c++14 -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall  -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -Wno-psabi

LDFLAGS+=-L/opt/vc/lib -lbcm_host -lvcos -lpthread -lrt -lm -lboost_system -lboost_filesystem -L../lib -ljson

INCLUDES+=-I../includes -I/opt/vc/include
CXX_FLAGS = -std=c++14 -pthread -O3 -DASIO_STANDALONE -Wtrigraphs 
CXX=g++-6

all : raspicam.bin starter
.PHONY : all


$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(OPTIONS) -MMD -c $< -o $@



starter: $(STARTER_OBJS)
	$(CXX) -o $@ -Wl,--whole-archive $(STARTER_OBJS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

raspicam.bin: $(OBJS)
	$(CXX) -o $@ -Wl,--whole-archive $(OBJS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic \
        -lcurl \
        -lopencv_features2d \
        -lopencv_highgui \
        -lopencv_videoio \
        -lopencv_imgcodecs \
        -lopencv_objdetect \
        -lopencv_imgproc \
        -lopencv_core

clean:
	for i in $(OBJS); do (if test -e "$$i"; then ( rm $$i ); fi ); done
	for i in $(STARTER_OBJS); do (if test -e "$$i"; then ( rm $$i ); fi ); done
	@rm  raspicam.bin starter 

.PHONY: clean
