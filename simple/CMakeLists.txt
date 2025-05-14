cmake_minimum_required(VERSION 3.14)
project(SimpleSegmentation)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure CPR options before fetching to avoid curl conflicts
set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "Use system curl instead of bundled curl" FORCE)
set(CPR_FORCE_USE_SYSTEM_CURL ON CACHE BOOL "Force using system curl" FORCE)
set(CPR_ENABLE_SSL ON CACHE BOOL "Enable SSL support" FORCE)

# Include FetchContent for downloading external dependencies
include(FetchContent)

# Fetch CPR library
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5  # Specify a specific version/tag
)
FetchContent_MakeAvailable(cpr)

# Fetch nlohmann-json library
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2  # Specify a specific version/tag
)
FetchContent_MakeAvailable(json)

# Find OpenCV package
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Find system curl for linking
find_package(CURL REQUIRED)

# Add the executable
add_executable(segmentation_client 
    main.cpp
    SimpleSegmentationClient.cpp
)

# Link against libraries
target_link_libraries(segmentation_client 
    PRIVATE cpr::cpr
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE ${OpenCV_LIBS}
    PRIVATE ${CURL_LIBRARIES}
)

# Add includes
target_include_directories(segmentation_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Installation
install(TARGETS segmentation_client DESTINATION bin)