BUILD_DIR := build
CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=Debug \
			   -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
			   #-DANDROID_SANITIZE=address \
			   #-DCMAKE_C_FLAGS="-g -fsanitize=address \
			   #-fno-omit-frame-pointer"
ANDROID_PLATFORM := android-34

android: android-libarm64-v8a android-libarmeabi-v7a
	. ./.env && $(MAKE) -f ./platform/android/apk.mk \
		BUILD_DIR=./$(BUILD_DIR)/apk \
		APP_SRC_DIR=./platform/android/app \
		LIB_DIR=./$(BUILD_DIR)/lib \
		ANDROID_PLATFORM=$(ANDROID_PLATFORM)

android-lib%:
	mkdir -p $(BUILD_DIR)
	cmake $(CMAKE_FLAGS) --preset android-$* \
		-S . -B ./$(BUILD_DIR)/$* \
		-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=./$(BUILD_DIR)/lib/$*
	cd ./$(BUILD_DIR)/$* && cmake --build .
	cp ./$(BUILD_DIR)/$*/compile_commands.json ./$(BUILD_DIR)

posixcli:
	mkdir -p $(BUILD_DIR)
	cmake $(CMAKE_FLAGS) -S . -B ./$(BUILD_DIR) -DNO_GUI=ON-S  && \
		cd ./$(BUILD_DIR) && \
		cmake --build .

clean:
	rm -rf $(BUILD_DIR)

