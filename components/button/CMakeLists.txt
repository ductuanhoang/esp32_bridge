
set(pri_req driver esp_timer)

set(BUTTON_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/button.c
)

set(include_dirs "include")

idf_component_register(SRCS ${BUTTON_SOURCES}
    INCLUDE_DIRS "${include_dirs}"
    REQUIRES ${pri_req})
