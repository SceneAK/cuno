PROJECT_DIR=${1:-"$HOME/prj/cuno"}
NDK=${2:-"$HOME/../usr/android-ndk-r27b"}
LIB=${3:-"libclang_rt.asan-aarch64-android.so"}

# This script is only for me lmao
cd $PROJECT_DIR && make clean && make && rm -r PROJECT_DIR/build/apk/
cp $NDK/toolchains/llvm/prebuilt/linux-aarch64/lib/clang/18/lib/linux/$LIB $PROJECT_DIR/build/app/lib/*/
cp $PROJECT_DIR/platform/android/wrap.sh $PROJECT_DIR/build/app/lib/*/
cd $PROJECT_DIR && make
