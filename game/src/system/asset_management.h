#ifndef ASSET_MANAGEMENT_H
#define ASSET_MANAGEMENT_H
#include <stddef.h>

#define ASSET_PATH_FONT "font/ProggyClean.ttf"

typedef void *asset_handle;

asset_handle asset_open(const char *asset_path);

void asset_close(asset_handle asset);

int asset_read(void *buffer, size_t len, asset_handle asset);

#endif
