#include <android/asset_manager.h>
#include "system/asset.h"
#include "system/log.h"
#include "asset_andr.h"

static AAssetManager *mngr = NULL;

/* Assumed to be called before all else. No checks are made for this. */
void asset_management_init(AAssetManager *asset_manager)
{
    mngr = asset_manager;
}

asset_handle asset_open(const char *asset_path)
{
    asset_handle handle = AAssetManager_open(mngr, asset_path, AASSET_MODE_BUFFER);
    if (!handle)
        LOGF(LOG_ERR, "ASSET: Asset Management failed te open resource \"%s\"", asset_path);
    return handle;
}

void asset_close(asset_handle handle)
{
    AAsset_close(handle);
}

int asset_read(void *buffer, size_t size, asset_handle handle)
{
    return AAsset_read(handle, buffer, size);
}
