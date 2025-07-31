set(COMPILED "${CMAKE_BINARY_DIR}/compiled.zip")
add_custom_command(
    OUTPUT COMPILED
    COMMAND aapt2 compile -o "${COMPILED}" --dir "${OUTPUT_APP_DIR}/res"
    DEPENDS cuno
    COMMENT "Compiling resources with aapt2"
    VERBATIM
)
set(UNSIGNED "${CMAKE_BINARY_DIR}/unsigned.apk")
add_custom_command(
    OUTPUT UNSIGNED
    COMMAND aapt2 link "${COMPILED}"
            -o "${UNSIGNED}"
	    -I "$ENV{ANDROID_SDK_ROOT}/platforms/${ANDROID_PLATFORM}/android.jar"
        -A "${OUTPUT_APP_DIR}/assets"
	    --manifest "${OUTPUT_APP_DIR}/AndroidManifest.xml"
    COMMAND cd "${OUTPUT_APP_DIR}" && zip -r "${UNSIGNED}" lib/
    DEPENDS COMPILED
    COMMENT "Linking resources to APK"
    VERBATIM
)
set(SIGNED "${CMAKE_BINARY_DIR}/cuno.apk")
add_custom_command(
    OUTPUT SIGNED
    COMMAND apksigner sign
            --ks $ENV{KS_KEY_JKS}
	    --ks-key-alias $ENV{KS_KEY_ALIAS}
	    --ks-pass env:KS_PASS
	    --key-pass env:KEY_PASS
	    --out "${SIGNED}"
	    "${UNSIGNED}"
    DEPENDS UNSIGNED
    COMMENT "Signing APK"
    VERBATIM
)
add_custom_target(cunoapk ALL DEPENDS SIGNED)
#add_custom_command(
#    TARGET cunoapk
#    COMMAND zipalign -v -p 4 signed.apk final-aligned.apk
#    DEPENDS signed.apk
#    COMMENT "Aligning APK"
#)
