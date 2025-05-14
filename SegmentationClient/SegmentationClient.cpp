
#include "SegmentationClient.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>

SegmentationClient::SegmentationClient(const std::string& server_url)
    : m_serverUrl(server_url) {
}

cv::Mat SegmentationClient::segmentImage(const cv::Mat& image) {
    // Make sure image is grayscale
    cv::Mat grayImage;
    if (image.channels() > 1) {
        cv::cvtColor(image, grayImage, cv::COLOR_BGR2GRAY);
    } else {
        grayImage = image.clone();
    }
    
    // Encode the image to PNG
    std::vector<uchar> imageBuffer = encodeImageToPNG(grayImage);
    
    // Create a temporary file
    std::string tempFilename = "temp_image.png";
    std::ofstream outFile(tempFilename, std::ios::binary);
    outFile.write(reinterpret_cast<const char*>(imageBuffer.data()), imageBuffer.size());
    outFile.close();
    
    // Send the image to the server using HTTP client
    cpr::Response response;
    
    try {
        cpr::Multipart multipart{{"image", cpr::File{tempFilename}}};
        response = cpr::Post(cpr::Url{m_serverUrl}, multipart);
    }
    catch (const std::exception& e) {
        std::cerr << "HTTP Error: " << e.what() << std::endl;
        std::remove(tempFilename.c_str());
        return cv::Mat();
    }
    
    // Clean up temporary file
    std::remove(tempFilename.c_str());
    
    // Check response status
    #ifdef USE_MINIMAL_HTTP_CLIENT
    if (response.status_code != 200 || !response.error.empty()) {
        std::cerr << "HTTP Error: " << response.status_code << std::endl;
        if (!response.error.empty()) {
            std::cerr << "Error message: " << response.error << std::endl;
        }
        return cv::Mat();
    }
    #else
    if (response.status_code != 200) {
        std::cerr << "HTTP Error: " << response.status_code << std::endl;
        std::cerr << "Error message: " << response.text << std::endl;
        return cv::Mat();
    }
    #endif
    // After getting the HTTP response, print the raw response text
    std::cout << "Server response: " << response.text.substr(0, 100) << "..." << std::endl;

    // Extract the base64 mask from the JSON response
    std::string base64Mask = extractBase64MaskFromJson(response.text);

    std::cout << "Base64 mask length: " << base64Mask.length() << std::endl;
    
    // Decode the base64 mask and convert to cv::Mat
    return decodeBase64Mask(base64Mask);
}

std::future<cv::Mat> SegmentationClient::segmentImageAsync(const cv::Mat& image) {
    // Create a promise to deliver the result
    std::shared_ptr<std::promise<cv::Mat>> resultPromise = 
        std::make_shared<std::promise<cv::Mat>>();
    
    // Get a future from the promise
    std::future<cv::Mat> resultFuture = resultPromise->get_future();
    
    // Create a copy of the image for the async task
    cv::Mat imageCopy = image.clone();
    
    // Launch the async task
    std::thread([this, imageCopy, resultPromise]() mutable {
        try {
            // Process the image synchronously within this thread
            cv::Mat result = this->segmentImage(imageCopy);
            
            // Set the promise value with the result
            resultPromise->set_value(result);
        } catch (const std::exception& e) {
            // Set the promise exception
            resultPromise->set_exception(std::current_exception());
        }
    }).detach();
    
    return resultFuture;
}

std::vector<uchar> SegmentationClient::encodeImageToPNG(const cv::Mat& image) {
    std::vector<uchar> buffer;
    std::vector<int> params = {cv::IMWRITE_PNG_COMPRESSION, 9}; // Maximum compression
    cv::imencode(".png", image, buffer, params);
    return buffer;
}

std::string SegmentationClient::extractBase64MaskFromJson(const std::string& jsonResponse) {
    try {
        // Parse the JSON response
        nlohmann::json responseJson = nlohmann::json::parse(jsonResponse);
        
        // Extract the first mask (assuming there's at least one)
        if (responseJson.contains("masks") && responseJson["masks"].is_array() && 
            !responseJson["masks"].empty()) {
            return responseJson["masks"][0].get<std::string>();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON response: " << e.what() << std::endl;
    }
    
    return "";
}

cv::Mat SegmentationClient::decodeBase64Mask(const std::string& base64Mask) {
    if (base64Mask.empty()) {
        return cv::Mat();
    }
    
    // Base64 decode
    std::string decoded;
    decoded.resize(base64Mask.size() * 3 / 4); // Approximate size
    
    // Create Base64 decoding table
    std::vector<unsigned char> decodingTable(256, -1);
    for (int i = 0; i < 64; i++) {
        unsigned char c = 0;
        if (i < 26) c = 'A' + i;
        else if (i < 52) c = 'a' + (i - 26);
        else if (i < 62) c = '0' + (i - 52);
        else if (i == 62) c = '+';
        else c = '/';
        
        decodingTable[c] = i;
    }
    
    int padding = 0;
    if (base64Mask.length() > 0 && base64Mask[base64Mask.length() - 1] == '=') padding++;
    if (base64Mask.length() > 1 && base64Mask[base64Mask.length() - 2] == '=') padding++;
    
    std::vector<uchar> data;
    for (size_t i = 0; i < base64Mask.length(); i += 4) {
        // Get values for each group of four characters
        unsigned char v1 = (i+0 < base64Mask.length() && base64Mask[i+0] != '=') ? decodingTable[base64Mask[i+0]] : 0;
        unsigned char v2 = (i+1 < base64Mask.length() && base64Mask[i+1] != '=') ? decodingTable[base64Mask[i+1]] : 0;
        unsigned char v3 = (i+2 < base64Mask.length() && base64Mask[i+2] != '=') ? decodingTable[base64Mask[i+2]] : 0;
        unsigned char v4 = (i+3 < base64Mask.length() && base64Mask[i+3] != '=') ? decodingTable[base64Mask[i+3]] : 0;
        
        // Write the first byte
        data.push_back((v1 << 2) | (v2 >> 4));
        
        // Write the second byte if there is enough data
        if (i+2 < base64Mask.length() && base64Mask[i+2] != '=')
            data.push_back(((v2 & 0x0F) << 4) | (v3 >> 2));
        
        // Write the third byte if there is enough data
        if (i+3 < base64Mask.length() && base64Mask[i+3] != '=')
            data.push_back(((v3 & 0x03) << 6) | v4);
    }
    
    // Decode the binary data to a Mat object
    return cv::imdecode(data, cv::IMREAD_UNCHANGED);
    
}
