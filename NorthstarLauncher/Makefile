
CXX=x86_64-w64-mingw32-g++

CXXFLAGS := -std=c++20
LDFLAGS := -static-libgcc -static-libstdc++ -lshlwapi -lwsock32 -lmsvcr120 -lws2_32 -Bstatic -lpthread

BUILD_DIR := linuxbuild
SRCS := main.cpp
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

$(BUILD_DIR)/NorthstarLauncher.exe: $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)


$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@


.PHONY: clean
clean:
	rm -r $(BUILD_DIR)
