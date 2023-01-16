
CXX=x86_64-w64-mingw32-g++

CXXFLAGS := -std=c++20 -fpermissive
LDFLAGS := -shared -static-libgcc -static-libstdc++ -lwsock32 -lws2_32 -L./minhook -lMinHook -Wl,-Bstatic -l:libz.dll.a -l:libnghttp2.dll.a -l:libcurl.dll.a -l:libcares.dll.a

BUILD_DIR := linuxbuild
SRCS := $(shell find . -type f -name '*.cpp')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
INCLUDES := $(addprefix -I,$(shell find . -type d))
INCLUDES += -I../include

debug: CXXFLAGS += -g
debug: all


all: $(BUILD_DIR)/Northstar.dll


$(BUILD_DIR)/Northstar.dll: $(OBJS) minhook/libMinHook.a
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)


$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) -c $< -o $@


minhook/libMinHook.a:
	$(MAKE) -C minhook -f build/MinGW/Makefile


.PHONY: clean
clean:
	rm -r $(BUILD_DIR)
	$(MAKE) -C minhook/build/MinGW clean
