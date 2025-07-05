#include "system/logging.h"
#include "system/graphic/graphic.h"
#include "main.h"

static struct graphic_session *session;
int init(struct graphic_session *created)
{
    session = created;
    return 0;
}

void update()
{
    if (session)
        graphic_draw(session);
}
