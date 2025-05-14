#pragma once

#include <opencv2/opencv.hpp>
#include <future>
#include <string>
#include <vector>

// Handle different ways to include nlohmann/json
#if __has_include(<nlohmann/json.hpp>)
    #include <nlohmann/json.hpp>
#elif __has_include("nlohmann/json.hpp")
    #include "nlohmann/json.hpp"
#else
    #error "nlohmann/json.hpp not found"
#endif

// Include the appropriate HTTP client
#ifdef USE_MINIMAL_HTTP_CLIENT
    #include "minimal_http_client.h"
#else
    #include <cpr/cpr.h>
#endif

class SegmentationClient {
public:
    SegmentationClient(const std::string& server_url = "http://192.248.10.70:8000/segment");
    
    // Synchronous request - blocks until completion
    cv::Mat segmentImage(const cv::Mat& image);
    
    // Asynchronous request using future/promise
    std::future<cv::Mat> segmentImageAsync(const cv::Mat& image);

private:
    std::string m_serverUrl;
    
    // Helper methods
    std::vector<uchar> encodeImageToPNG(const cv::Mat& image);
    cv::Mat decodeBase64Mask(const std::string& base64Mask);
    std::string extractBase64MaskFromJson(const std::string& jsonResponse);
};
