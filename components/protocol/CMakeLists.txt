set(pri_req ArduinoJson esp_event driver esp_wifi)

# Set the directories that should be included in the build command for this target
# when running g++ these will be included as -I/directory/path/
message(CMAKE_CURRENT_LIST_DIR="${CMAKE_CURRENT_LIST_DIR}")

set(USER_MESSAGE
    ${CMAKE_CURRENT_LIST_DIR}/src/UDP.cpp
)

set(include_dirs "include")

idf_component_register(SRCS ${USER_MESSAGE}
    INCLUDE_DIRS "${include_dirs}"
    REQUIRES ${pri_req})