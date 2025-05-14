#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>

using json = nlohmann::json;

class YOLOSegmenterClient {
private:
    std::string server_url;

public:
    YOLOSegmenterClient(const std::string& url) : server_url(url) {}

    // Function to decode base64 string to binary data
    std::vector<uchar> decodeBase64(const std::string& encoded_string) {
        static const std::string base64_chars = 
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        
        std::vector<uchar> decoded;
        int i = 0, j = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::string input = encoded_string;
        
        // Remove any non-base64 characters like whitespace
        input.erase(std::remove_if(input.begin(), input.end(), [&](char c) {
            return base64_chars.find(c) == std::string::npos && c != '=';
        }), input.end());
        
        int in_len = input.size();
        
        while (in_len-- && input[i] != '=') {
            char_array_4[j++] = input[i++];
            if (j == 4) {
                for (j = 0; j < 4; j++)
                    char_array_4[j] = base64_chars.find(char_array_4[j]);
                
                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
                
                for (j = 0; j < 3; j++)
                    decoded.push_back(char_array_3[j]);
                j = 0;
            }
        }
        
        if (j) {
            for (int k = j; k < 4; k++)
                char_array_4[k] = 0;
            
            for (int k = 0; k < 4; k++)
                char_array_4[k] = base64_chars.find(char_array_4[k]);
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (int k = 0; k < j - 1; k++)
                decoded.push_back(char_array_3[k]);
        }
        
        return decoded;
    }

    std::future<std::vector<cv::Mat>> fetchMasksAsync(const cv::Mat& image) {
        return std::async(std::launch::async, [this, image]() {
            std::vector<cv::Mat> masks;
            
            // Save image to temporary file
            std::string temp_file = "temp_image.jpg";
            cv::imwrite(temp_file, image);
            
            // Prepare multipart request for the YOLO server
            // Note: Your server expects "image" as the field name, not "file"
            cpr::Multipart multipart{
                {"image", cpr::File{temp_file}}
            };
            
            // Make asynchronous POST request
            std::cout << "Sending request to " << server_url << std::endl;
            auto future_response = cpr::PostAsync(
                cpr::Url{server_url},
                multipart
            );
            
            // Wait for response
            auto response = future_response.get();
            
            // Clean up temporary file
            std::remove(temp_file.c_str());
            
            // Check if request was successful
            if (response.status_code != 200) {
                std::cerr << "Error: " << response.status_code << " - " << response.text << std::endl;
                return masks;
            }
            
            // Parse JSON response
            try {
                auto j = json::parse(response.text);
                
                // Extract masks from response - your server returns an array of masks
                auto masks_array = j["masks"].get<std::vector<std::string>>();
                
                std::cout << "Received " << masks_array.size() << " masks from server" << std::endl;
                
                // Process each mask
                for (const auto& base64_mask : masks_array) {
                    // Decode base64 to binary
                    std::vector<uchar> mask_data = decodeBase64(base64_mask);
                    
                    // Decode mask from binary data
                    cv::Mat mask = cv::imdecode(mask_data, cv::IMREAD_UNCHANGED);
                    
                    if (!mask.empty()) {
                        masks.push_back(mask);
                    }
                }
                
                return masks;
            } catch (const std::exception& e) {
                std::cerr << "Error parsing response: " << e.what() << std::endl;
                return masks;
            }
        });
    }
    
    // Create a combined mask from multiple individual masks
    cv::Mat createCombinedMask(const std::vector<cv::Mat>& masks, const cv::Size& image_size) {
        cv::Mat combined_mask = cv::Mat::zeros(image_size, CV_8UC1);
        
        for (const auto& mask : masks) {
            // Resize mask if needed
            cv::Mat resized_mask;
            if (mask.size() != image_size) {
                cv::resize(mask, resized_mask, image_size, 0, 0, cv::INTER_NEAREST);
            } else {
                resized_mask = mask;
            }
            
            // Combine masks (logical OR)
            cv::bitwise_or(combined_mask, resized_mask, combined_mask);
        }
        
        return combined_mask;
    }
};

// Helper function to extract mask coordinates
std::vector<cv::Point> getMaskCoordinates(const cv::Mat& mask, int threshold = 128) {
    std::vector<cv::Point> coordinates;
    
    // Make sure the mask is valid
    if (mask.empty()) {
        std::cerr << "Error: Empty mask" << std::endl;
        return coordinates;
    }
    
    // Convert mask to binary if needed
    cv::Mat binary_mask;
    if (mask.channels() > 1) {
        cv::cvtColor(mask, binary_mask, cv::COLOR_BGR2GRAY);
    } else {
        binary_mask = mask.clone();
    }
    
    if (binary_mask.type() != CV_8U) {
        binary_mask.convertTo(binary_mask, CV_8U);
    }
    
    // Threshold the mask if needed
    cv::Mat thresholded_mask;
    cv::threshold(binary_mask, thresholded_mask, threshold, 255, cv::THRESH_BINARY);
    
    // Extract coordinates of non-zero pixels
    for (int y = 0; y < thresholded_mask.rows; ++y) {
        for (int x = 0; x < thresholded_mask.cols; ++x) {
            if (thresholded_mask.at<uchar>(y, x) > 0) {
                coordinates.push_back(cv::Point(x, y));
            }
        }
    }
    
    return coordinates;
}

// Visualize mask overlaid on the original image
cv::Mat visualizeMask(const cv::Mat& image, const cv::Mat& mask, const cv::Scalar& color = cv::Scalar(0, 0, 255)) {
    cv::Mat visualization;
    image.copyTo(visualization);
    
    // Create a colored overlay
    cv::Mat colored_mask = cv::Mat::zeros(image.size(), CV_8UC3);
    for (int y = 0; y < mask.rows; ++y) {
        for (int x = 0; x < mask.cols; ++x) {
            if (mask.at<uchar>(y, x) > 0) {
                colored_mask.at<cv::Vec3b>(y, x) = cv::Vec3b(color[0], color[1], color[2]);
            }
        }
    }
    
    // Blend the original image with the colored mask
    cv::addWeighted(visualization, 0.7, colored_mask, 0.3, 0, visualization);
    
    return visualization;
}

int main(int argc, char** argv) {
    // Check if image path is provided
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <image_path> [server_url]" << std::endl;
        return 1;
    }
    
    // Load image
    std::string image_path = argv[1];
    cv::Mat image = cv::imread(image_path);
    
    if (image.empty()) {
        std::cerr << "Error: Could not load image: " << image_path << std::endl;
        return 1;
    }
    
    // Set server URL (default or from command line)
    std::string server_url = "http://192.248.10.70:8000/segment";
    if (argc > 2) {
        server_url = argv[2];
    }
    
    // Create segmenter client
    YOLOSegmenterClient client(server_url);
    
    // Start time measurement
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Send request asynchronously
    std::cout << "Sending request to " << server_url << "..." << std::endl;
    auto future_masks = client.fetchMasksAsync(image);
    
    // Do other work while waiting for the response
    std::cout << "Request sent. Processing other tasks while waiting..." << std::endl;
    for (int i = 0; i < 5; ++i) {
        std::cout << "Processing task " << i + 1 << "..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    // Get the masks when ready
    std::cout << "Waiting for segmentation result..." << std::endl;
    std::vector<cv::Mat> masks = future_masks.get();
    
    // End time measurement
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "Request completed in " << duration << "ms" << std::endl;
    
    // Check if masks were received
    if (masks.empty()) {
        std::cerr << "Error: No masks received from server" << std::endl;
        return 1;
    }
    
    std::cout << "Received " << masks.size() << " masks" << std::endl;
    
    // Create a combined mask from all individual masks
    cv::Mat combined_mask = client.createCombinedMask(masks, image.size());
    
    // Save individual masks
    for (size_t i = 0; i < masks.size(); ++i) {
        std::string mask_path = "mask_" + std::to_string(i) + ".png";
        cv::imwrite(mask_path, masks[i]);
        std::cout << "Saved mask " << i << " to " << mask_path << std::endl;
    }
    
    // Save combined mask
    std::string combined_mask_path = "combined_mask.png";
    cv::imwrite(combined_mask_path, combined_mask);
    std::cout << "Saved combined mask to " << combined_mask_path << std::endl;
    
    // Get mask coordinates from combined mask
    std::vector<cv::Point> coordinates = getMaskCoordinates(combined_mask);
    std::cout << "Found " << coordinates.size() << " mask points" << std::endl;
    
    // Display first few coordinates
    int num_to_display = std::min(10, static_cast<int>(coordinates.size()));
    std::cout << "First " << num_to_display << " coordinates:" << std::endl;
    for (int i = 0; i < num_to_display; ++i) {
        std::cout << "(" << coordinates[i].x << ", " << coordinates[i].y << ")" << std::endl;
    }
    
    // Create visualization
    cv::Mat visualization = visualizeMask(image, combined_mask);
    
    // Display results
    cv::imshow("Original Image", image);
    cv::imshow("Combined Mask", combined_mask);
    cv::imshow("Visualization", visualization);
    
    // Wait for key press to close windows
    cv::waitKey(0);
    
    return 0;
}