# Inputs
BUILD_DIR    ?= apk
APP_SRC_DIR  ?=
APKNAME      ?= cuno
ANDROID_PLATFORM ?= android-34
ANDROID_JAR  ?= $(ANDROID_SDK_ROOT)/platforms/$(ANDROID_PLATFORM)/android.jar

# Sources
RES_DIR  	 ?= $(APP_SRC_DIR)/res
ASSET_DIR  	 ?= $(APP_SRC_DIR)/assets
LIB_DIR    	 ?= $(APP_SRC_DIR)/lib
MANIFEST     ?= $(APP_SRC_DIR)/AndroidManifest.xml


# Tools
AAPT2     	 := aapt2
APKSIGNER 	 := apksigner
ZIP       	 := zip

# Output Paths
COMPILED_RES := $(BUILD_DIR)/compiled.zip
UNSIGNED_APK := $(BUILD_DIR)/unsigned.apk
SIGNED_APK   := $(BUILD_DIR)/$(APKNAME).apk

.PHONY: all clean

all: $(SIGNED_APK)

$(COMPILED_RES): $(RES_DIR)
	@mkdir -p $(BUILD_DIR)
	@echo "==> Compiling resources..."
	$(AAPT2) compile -o $@ --dir $(RES_DIR)

$(UNSIGNED_APK): $(COMPILED_RES) $(MANIFEST) $(ASSET_FILES) $(LIB_FILES)
	@echo "==> Linking resources..."
	$(AAPT2) link $(COMPILED_RES) \
		-o $@ \
		-I $(ANDROID_JAR) \
		-A $(ASSET_DIR) \
		--manifest $(MANIFEST)
	
	@echo "==> Adding libs..."
	cd "$(dir $(LIB_DIR))" && $(ZIP) -u -r "$(abspath $@)" ./lib/; \

$(SIGNED_APK): $(UNSIGNED_APK)
	@echo "==> Signing APK..."
	$(APKSIGNER) sign \
		--ks "$(KS_KEY_JKS)" \
		--ks-key-alias "$(KS_KEY_ALIAS)" \
		--ks-pass env:KS_PASS \
		--key-pass env:KEY_PASS \
		--out $@ \
		--in $(UNSIGNED_APK)
	@echo "==> Build Success: $@"

clean:
	@echo "==> Cleaning build directory..."
	rm -rf $(BUILD_DIR)
