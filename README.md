# CUNO
A card game inspired by UNO, written in C, developed entirely on my phone!

## Android Build
1. Download the NDK & SDK(.jar)
2. Copy .env.template as ".env", edit accordingly
3. Run `make android`
4. Install with adb or the `inst_run_apk.sh` script

## Debug CLI Build
1. Run `make posixcli`
2. Usage: 
    - `cuno <ip> <port>` - run as client
    - `cuno <port>` - run as host
