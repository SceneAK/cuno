#include "game.h"
#include "system/log.h"

int main()
{
    LOGF("game_init: %d", game_init(NULL));
    while(1) {
        game_update();
    }
    return 0;
}
