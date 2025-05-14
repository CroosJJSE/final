#include "SimpleSegmentationClient.h"
#include <iostream>
#include <chrono>
#include <thread>

int main(int argc, char* argv[]) {
    // Default image path and server URL
    std::string imagePath = "input.jpg";
    std::string serverUrl = "http://192.248.10.70:8000/segment";
    
    // Parse command line arguments
    if (argc > 1) {
        imagePath = argv[1];
    }
    if (argc > 2) {
        serverUrl = argv[2];
    }
    
    // Load the image
    cv::Mat image = cv::imread(imagePath);
    if (image.empty()) {
        std::cerr << "Error: Could not load image from " << imagePath << std::endl;
        return 1;
    }
    
    std::cout << "Image loaded: " << imagePath << std::endl;
    std::cout << "Using server: " << serverUrl << std::endl;
    
    // Create segmentation client
    SimpleSegmentationClient client(serverUrl);
    
    // Example 1: Synchronous segmentation
    std::cout << "\n--- Example 1: Synchronous segmentation ---" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    
    cv::Mat mask = client.segmentImage(image);
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    
    if (!mask.empty()) {
        std::cout << "Segmentation completed in " << duration << "ms" << std::endl;
        
        // Extract coordinates from mask
        std::vector<cv::Point> coordinates = client.extractCoordinatesFromMask(mask);
        std::cout << "Found " << coordinates.size() << " points in the mask" << std::endl;
        
        // Save result image
        cv::Mat result = image.clone();
        cv::Mat colorMask(mask.size(), CV_8UC3, cv::Scalar(0, 255, 0));
        cv::bitwise_and(colorMask, colorMask, colorMask, mask);
        cv::addWeighted(result, 0.7, colorMask, 0.3, 0, result);
        cv::imwrite("result_sync.jpg", result);
        cv::imwrite("mask_sync.jpg", mask);
        
        std::cout << "Saved result to 'result_sync.jpg' and mask to 'mask_sync.jpg'" << std::endl;
    } else {
        std::cout << "Segmentation failed!" << std::endl;
    }
    
    // Example 2: Asynchronous segmentation
    std::cout << "\n--- Example 2: Asynchronous segmentation ---" << std::endl;
    start = std::chrono::high_resolution_clock::now();
    
    // Start the async segmentation
    std::future<cv::Mat> future = client.segmentImageAsync(image);
    
    // While waiting for the result, we can do other work
    std::cout << "Segmentation request sent, doing other work while waiting..." << std::endl;
    for (int i = 0; i < 5; i++) {
        std::cout << "Doing work: " << i + 1 << "/5" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    // Check if the result is ready
    if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        std::cout << "Result already available!" << std::endl;
    } else {
        std::cout << "Result not yet available, waiting for it..." << std::endl;
    }
    
    // Get the result (this will block until the result is available)
    cv::Mat asyncMask = future.get();
    
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    
    if (!asyncMask.empty()) {
        std::cout << "Async segmentation completed in " << duration << "ms" << std::endl;
        
        // Extract coordinates from mask
        std::vector<cv::Point> coordinates = client.extractCoordinatesFromMask(asyncMask);
        std::cout << "Found " << coordinates.size() << " points in the mask" << std::endl;
        
        // Print first 5 coordinates (if available)
        std::cout << "First coordinates: ";
        for (size_t i = 0; i < std::min(coordinates.size(), size_t(5)); i++) {
            std::cout << "(" << coordinates[i].x << "," << coordinates[i].y << ") ";
        }
        std::cout << std::endl;
        
        // Save result image
        cv::Mat result = image.clone();
        cv::Mat colorMask(asyncMask.size(), CV_8UC3, cv::Scalar(0, 255, 0));
        cv::bitwise_and(colorMask, colorMask, colorMask, asyncMask);
        cv::addWeighted(result, 0.7, colorMask, 0.3, 0, result);
        cv::imwrite("result_async.jpg", result);
        cv::imwrite("mask_async.jpg", asyncMask);
        
        std::cout << "Saved result to 'result_async.jpg' and mask to 'mask_async.jpg'" << std::endl;
    } else {
        std::cout << "Async segmentation failed!" << std::endl;
    }
    
    return 0;
}