
BUILD_DIR := linuxbuild/

all: $(BUILD_DIR)/NorthstarLauncher.exe $(BUILD_DIR)/Northstar.dll
	cp NorthstarDLL/$(BUILD_DIR)/Northstar.dll
	cp NorthstarLauncher/$(BUILD_DIR)/NorthstarLauncher.exe

$(BUILD_DIR)/NorthstarLauncher.exe:
	$(MAKE) -C NorthstarLauncher/


$(BUILD_DIR)/Northstar.dll:
	$(MAKE) -C NorthstarDLL/


.PHONY: clean
clean:
	rm -r $(BUILD_DIR)
	$(MAKE) -C NorthstarDLL/ clean
	$(MAKE) -C NorthstarLauncher/
