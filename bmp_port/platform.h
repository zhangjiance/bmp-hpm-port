#include "board.h"

#define PLATFORM_IDENT   "(HPMicro-5301)"

#define SET_RUN_STATE(state)  \
{ \
    running_status = state; \
}  
#define SET_IDLE_STATE(state)  \
{ \
    if (state) { \
        running_status = 0; \
        board_led_write(!state); \
    } else { \
        running_status = 1; \
    } \
}

#define SET_ERROR_STATE(state) 