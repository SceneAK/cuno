struct graphic_session;

struct graphic_session *graphic_session_create();

int graphic_session_switch_window(struct graphic_session *session, void *native_window_handle);

int graphic_session_destroy(struct graphic_session *session);

void graphic_gay_test(struct graphic_session *session);
