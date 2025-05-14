cmake_minimum_required(VERSION 3.10)
project(SegmentationClient)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV package
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Find OpenSSL (required for CPR)
find_package(OpenSSL QUIET)
if(NOT OpenSSL_FOUND)
    message(STATUS "OpenSSL not found. Some features may not work.")
endif()

# Try to find nlohmann_json package using different methods
# Method 1: Find installed package
find_package(nlohmann_json 3.9.0 QUIET)

if(NOT nlohmann_json_FOUND)
    # Method 2: Check for the package without version
    find_package(nlohmann_json QUIET)
endif()

if(NOT nlohmann_json_FOUND)
    # Method 3: Check if it's available as "json"
    find_package(json QUIET)
endif()

# If still not found, download it automatically
if(NOT nlohmann_json_FOUND AND NOT json_FOUND)
    message(STATUS "nlohmann_json not found. Downloading it...")
    include(FetchContent)
    FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
    )
    FetchContent_MakeAvailable(json)
    set(JSON_INCLUDE_DIR ${CMAKE_BINARY_DIR}/_deps/json-src/include)
    include_directories(${JSON_INCLUDE_DIR})
    message(STATUS "nlohmann_json has been downloaded to ${JSON_INCLUDE_DIR}")
endif()

# Find thread package
find_package(Threads REQUIRED)

# Try a different approach for CPR
# Option 1: Find already installed CPR
find_package(cpr QUIET)

# Option 2: CPR not found, create a minimal HTTP client alternative
if(NOT cpr_FOUND)
    message(STATUS "CPR not found. Using a minimal CURL-based implementation instead.")
    
    # Find CURL directly
    find_package(CURL REQUIRED)
    include_directories(${CURL_INCLUDE_DIRS})
    
    # Create a directory for our custom implementation
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/custom_http_client)
    
    # Create a minimal HTTP client implementation
    file(WRITE ${CMAKE_BINARY_DIR}/custom_http_client/minimal_http_client.h [=[
#pragma once
#include <curl/curl.h>
#include <string>
#include <map>
#include <vector>

namespace minimal_http {

// Simple response structure
struct Response {
    long status_code;
    std::string text;
    std::string error;
};

// Callback to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Simplified POST request function
inline Response Post(const std::string& url, const std::map<std::string, std::string>& parameters,
                   const std::map<std::string, std::string>& files = {}) {
    CURL* curl = curl_easy_init();
    Response response = {0, "", ""};
    
    if (curl) {
        // Form data for parameters and files
        struct curl_httppost* formpost = NULL;
        struct curl_httppost* lastptr = NULL;
        
        // Add parameters to form
        for (const auto& param : parameters) {
            curl_formadd(&formpost, &lastptr,
                        CURLFORM_COPYNAME, param.first.c_str(),
                        CURLFORM_COPYCONTENTS, param.second.c_str(),
                        CURLFORM_END);
        }
        
        // Add files to form
        for (const auto& file : files) {
            curl_formadd(&formpost, &lastptr,
                        CURLFORM_COPYNAME, file.first.c_str(),
                        CURLFORM_FILE, file.second.c_str(),
                        CURLFORM_END);
        }
        
        // Set options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.text);
        
        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        
        // Check for errors
        if (res != CURLE_OK) {
            response.error = curl_easy_strerror(res);
        }
        
        // Get the status code
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status_code);
        
        // Clean up
        curl_formfree(formpost);
        curl_easy_cleanup(curl);
    }
    
    return response;
}

// Helper namespace for URL parameters (not needed for our specific case)
namespace Url {
    struct Params {
        std::map<std::string, std::string> parameters;
    };
}

// Helper class to match CPR API style
class File {
public:
    File(const std::string& filepath) : filepath_(filepath) {}
    const std::string& filepath() const { return filepath_; }
private:
    std::string filepath_;
};

// Helper class to match CPR API style
class Multipart {
public:
    Multipart(const std::initializer_list<std::pair<const std::string, File>>& parts) {
        for (const auto& part : parts) {
            files_[part.first] = part.second.filepath();
        }
    }
    
    const std::map<std::string, std::string>& files() const { return files_; }
private:
    std::map<std::string, std::string> files_;
};

} // namespace minimal_http

// Define a namespace alias to make usage similar to CPR
namespace cpr {
    using Response = minimal_http::Response;
    using File = minimal_http::File;
    using Multipart = minimal_http::Multipart;
    namespace Url {
        using Params = minimal_http::Url::Params;
    }
    
    inline Response Post(const std::string& url, const Multipart& multipart) {
        return minimal_http::Post(url, {}, multipart.files());
    }
    
    // Simple structure to match CPR API
    struct Url {
        std::string url;
        Url(const std::string& u) : url(u) {}
        operator std::string() const { return url; }
    };
}
]=])

    # Add include path
    include_directories(${CMAKE_BINARY_DIR}/custom_http_client)
endif()

# Add source files
set(SOURCES
    main.cpp
    SegmentationClient.cpp
    IPCameraCapture.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})



# Link libraries
target_link_libraries(${PROJECT_NAME} 
    ${OpenCV_LIBS}
    Threads::Threads
)

# Link with CURL if using minimal implementation
if(NOT cpr_FOUND)
    target_link_libraries(${PROJECT_NAME} ${CURL_LIBRARIES})
    add_definitions(-DUSE_MINIMAL_HTTP_CLIENT)
else()
    # Link with CPR
    target_link_libraries(${PROJECT_NAME} cpr::cpr)
endif()

# Link with nlohmann_json if found as a package
if(nlohmann_json_FOUND)
    target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)
elseif(json_FOUND)
    target_link_libraries(${PROJECT_NAME} nlohmann_json)
endif()

# Installation
install(TARGETS ${PROJECT_NAME} DESTINATION bin)