#include <android/asset_manager.h>
#include "engine/system/asset.h"
#include "engine/system/log.h"
#include "engine/system/asset/asset_andr.h"

static AAssetManager *mngr = NULL;

void asset_management_init(AAssetManager *asset_manager)
{
    mngr = asset_manager;
}

asset_handle asset_open(const char *asset_path)
{
    asset_handle handle = AAssetManager_open(mngr, asset_path, AASSET_MODE_BUFFER);
    if (!handle)
        cuno_logf(LOG_ERR, "ASSET: Asset Management failed te open resource \"%s\"", asset_path);
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
