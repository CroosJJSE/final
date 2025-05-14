#ifndef SIMPLE_SEGMENTATION_CLIENT_H
#define SIMPLE_SEGMENTATION_CLIENT_H

#include <string>
#include <vector>
#include <future>
#include <opencv2/opencv.hpp>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

class SimpleSegmentationClient {
public:
    SimpleSegmentationClient(const std::string& server_url);
    
    // Synchronous method
    cv::Mat segmentImage(const cv::Mat& image);
    
    // Asynchronous method
    std::future<cv::Mat> segmentImageAsync(const cv::Mat& image);
    
    // Extract coordinates from mask
    std::vector<cv::Point> extractCoordinatesFromMask(const cv::Mat& mask, int threshold = 127);

private:
    std::string m_serverUrl;
    
    // Helper methods
    std::vector<uchar> encodeImageToPNG(const cv::Mat& image);
    std::string extractBase64MaskFromJson(const std::string& jsonResponse);
    cv::Mat decodeBase64Mask(const std::string& base64Mask);
};

#endif // SIMPLE_SEGMENTATION_CLIENT_H