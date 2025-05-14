cmake_minimum_required(VERSION 3.10)
project(YOLOSegmenterClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch CPR library
include(FetchContent)
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 871ed52d350214a034f6ef8a3b8f51c5ce1bd400 # v1.9.0
)
FetchContent_MakeAvailable(cpr)

# Fetch nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# Find OpenCV
find_package(OpenCV REQUIRED)

# Include directories
include_directories(${OpenCV_INCLUDE_DIRS})

# Add executable
add_executable(yolo_segmenter_client main.cpp)

# Link libraries
target_link_libraries(yolo_segmenter_client PRIVATE 
    cpr::cpr
    nlohmann_json::nlohmann_json
    ${OpenCV_LIBS}
)