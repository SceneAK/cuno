BUILD_DIR := build
CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON #-DANDROID_SANITIZE=address -DCMAKE_C_FLAGS="-g -fsanitize=address -fno-omit-frame-pointer"
ABI := aarch64
NO_GUI := off

android:
	mkdir -p $(BUILD_DIR)
	. ./.env && \
		cmake $(CMAKE_FLAGS) --preset android_${ABI} -S . -B $(BUILD_DIR) -DNO_GUI=${NO_GUI} && \
		cd $(BUILD_DIR) && \
		cmake --build .

posixcli:
	mkdir -p $(BUILD_DIR)
	cmake $(CMAKE_FLAGS) -S . -B $(BUILD_DIR) -DNO_GUI=ON-S  && \
		cd $(BUILD_DIR) && \
		cmake --build .

clean:
	rm -rf $(BUILD_DIR)

