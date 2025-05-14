#include "SegmentationClient.h"
#include <iostream>
#include <string>

// Simple test program to send a single image to the segmentation server
int main(int argc, char* argv[]) {
    // Default server URL - same as in your main application
    std::string serverUrl = "http://192.248.10.70:8000/segment";
    
    // Default image path - you'll need to change this to a real image on your system
    std::string imagePath = "test_image.jpg";
    
    // Parse command line arguments
    if (argc > 1) {
        imagePath = argv[1];
    }
    
    if (argc > 2) {
        serverUrl = argv[2];
    }
    
    std::cout << "Testing segmentation with image: " << imagePath << std::endl;
    std::cout << "Server URL: " << serverUrl << std::endl;
    
    // Create the segmentation client
    SegmentationClient client(serverUrl);
    
    // Load the test image
    cv::Mat testImage = cv::imread(imagePath, cv::IMREAD_GRAYSCALE);
    if (testImage.empty()) {
        std::cerr << "Error: Could not load image: " << imagePath << std::endl;
        return 1;
    }
    
    std::cout << "Loaded image: " << testImage.cols << "x" << testImage.rows << std::endl;
    
    // Display the original image
    cv::imshow("Test Image", testImage);
    
    // Add detailed logging
    std::cout << "Sending image to server..." << std::endl;
    
    // Send the image for segmentation
    cv::Mat segmentationMask = client.segmentImage(testImage);
    
    // Check the result
    if (segmentationMask.empty()) {
        std::cerr << "Error: Received empty segmentation mask" << std::endl;
        return 1;
    }
    
    std::cout << "Received segmentation mask: " 
              << segmentationMask.rows << "x" << segmentationMask.cols 
              << ", non-zero pixels: " << cv::countNonZero(segmentationMask) << std::endl;
    
    // Display the segmentation mask
    cv::imshow("Segmentation Mask", segmentationMask);
    
    // Apply the mask to the original image for visualization
    cv::Mat visualizedSegmentation;
    cv::cvtColor(testImage, visualizedSegmentation, cv::COLOR_GRAY2BGR);
    
    // Create a colored overlay for the mask
    cv::Mat colorMask = cv::Mat::zeros(segmentationMask.size(), CV_8UC3);
    colorMask.setTo(cv::Scalar(0, 255, 0), segmentationMask > 0);
    
    // Blend the original image with the colored mask
    cv::addWeighted(visualizedSegmentation, 0.7, colorMask, 0.3, 0, visualizedSegmentation);
    
    // Display the blended result
    cv::imshow("Segmentation Result", visualizedSegmentation);
    
    // Save results to disk
    cv::imwrite("test_result_original.jpg", testImage);
    cv::imwrite("test_result_mask.jpg", segmentationMask);
    cv::imwrite("test_result_visualization.jpg", visualizedSegmentation);
    
    std::cout << "Saved results to disk" << std::endl;
    std::cout << "Press any key to exit..." << std::endl;
    
    // Wait for a key press
    cv::waitKey(0);
    
    return 0;
}