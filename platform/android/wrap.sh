#!/system/bin/sh
HERE="$(cd "$(dirname "$0")" && pwd)"
cmd=$1
shift

# Set ASAN_OPTIONS for configuration (optional, but common)
export ASAN_OPTIONS=log_to_syslog=false,allow_user_segv_handler=1

# Determine the correct ASan library name for LD_PRELOAD
ASAN_LIB=$(ls "$HERE"/libclang_rt.asan-*-android.so)

# LD_PRELOAD forces the ASan runtime to load before the application starts
export LD_PRELOAD="$ASAN_LIB"

# Execute the original application command
exec "$cmd" "$@"
