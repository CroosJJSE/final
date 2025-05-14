#include "SegmentationClient.h"
#include "IPCameraCapture.h"
#include <iostream>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>

class SegmentationPipeline {
public:
    SegmentationPipeline(const std::string& cameraUrl, const std::string& serverUrl)
        : m_camera(cameraUrl), 
          m_segmentationClient(serverUrl),
          m_isRunning(false),
          m_processingQueueSize(3),  // Max number of frames in processing queue
          m_showVisualization(true) {
    }
    
    bool start() {
        if (m_isRunning) {
            return true;
        }
        
        // Set the camera resolution to match the required dimensions
        m_camera.setResolution(600, 350);
        
        // Set the frame callback
        m_camera.setFrameCallback([this](const cv::Mat& frame) {
            this->processFrame(frame);
        });
        
        // Start the camera
        if (!m_camera.start()) {
            std::cerr << "Failed to start camera" << std::endl;
            return false;
        }
        
        // Start the processing thread
        m_isRunning = true;
        m_processingThread = std::thread(&SegmentationPipeline::processingLoop, this);
        
        return true;
    }
    
    void stop() {
        if (!m_isRunning) {
            return;
        }
        
        // Signal the thread to stop
        m_isRunning = false;
        
        // Stop the camera
        m_camera.stop();
        
        // Notify the processing thread
        m_queueCondition.notify_all();
        
        // Wait for the thread to finish
        if (m_processingThread.joinable()) {
            m_processingThread.join();
        }
        
        // Close OpenCV windows
        cv::destroyAllWindows();
    }
    
    void setShowVisualization(bool show) {
        m_showVisualization = show;
    }
    
private:
    void processFrame(const cv::Mat& frame) {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        
        // Check if the queue is full
        if (m_frameQueue.size() >= m_processingQueueSize) {
            // Remove the oldest frame
            m_frameQueue.pop();
        }
        
        // Add the new frame to the queue
        m_frameQueue.push(frame.clone());
        
        // Notify the processing thread
        lock.unlock();
        m_queueCondition.notify_one();
    }
    
    void processingLoop() {
        while (m_isRunning) {
            cv::Mat frame;
            
            // Get a frame from the queue
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_queueCondition.wait(lock, [this] { 
                    return !m_frameQueue.empty() || !m_isRunning; 
                });
                
                if (!m_isRunning) {
                    break;
                }
                
                if (m_frameQueue.empty()) {
                    continue;
                }
                
                frame = m_frameQueue.front();
                m_frameQueue.pop();
            }
            
            // Convert to grayscale if necessary
            cv::Mat grayFrame;
            if (frame.channels() > 1) {
                cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);
            } else {
                grayFrame = frame;
            }
            
            // Start the segmentation timer
            auto startTime = std::chrono::high_resolution_clock::now();
            
            // Send the frame for segmentation asynchronously
            std::future<cv::Mat> maskFuture = m_segmentationClient.segmentImageAsync(grayFrame);
            
            // Wait for the result
            cv::Mat segmentationMask = maskFuture.get();
            // Inside processingLoop() after getting the segmentation mask
            if (!segmentationMask.empty()) {
                std::cout << "Received segmentation mask: " 
                        << segmentationMask.rows << "x" << segmentationMask.cols 
                        << ", non-zero pixels: " << cv::countNonZero(segmentationMask) << std::endl;
            } else {
                std::cout << "Received empty segmentation mask" << std::endl;
            }


            // Calculate processing time
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
            
            // Display the results if visualization is enabled
            if (m_showVisualization && !segmentationMask.empty()) {
                // Display the original grayscale image
                cv::imshow("Original Image", grayFrame);
                
                // Display the segmentation mask
                cv::imshow("Segmentation Mask", segmentationMask);
                
                // Apply the mask to the original image for visualization
                cv::Mat visualizedSegmentation;
                cv::cvtColor(grayFrame, visualizedSegmentation, cv::COLOR_GRAY2BGR);
                
                // Create a colored overlay for the mask
                cv::Mat colorMask = cv::Mat::zeros(segmentationMask.size(), CV_8UC3);
                colorMask.setTo(cv::Scalar(0, 255, 0), segmentationMask > 0);
                
                // Blend the original image with the colored mask
                cv::addWeighted(visualizedSegmentation, 0.7, colorMask, 0.3, 0, visualizedSegmentation);
                
                // Display the blended result
                cv::imshow("Segmentation Result", visualizedSegmentation);
                
                // Show processing time
                std::cout << "Segmentation processing time: " << duration << " ms" << std::endl;
                
                            // Inside processingLoop() where visualization would happen
                if (!segmentationMask.empty()) {
                    // Save images to disk for verification
                    static int frameCount = 0;
                    std::string baseName = "frame_" + std::to_string(frameCount++);
                    
                    cv::imwrite(baseName + "_original.jpg", grayFrame);
                    cv::imwrite(baseName + "_mask.jpg", segmentationMask);
                    
                    // Print confirmation
                    std::cout << "Saved frame " << frameCount << " to disk" << std::endl;
                }
                // Process keyboard input
                int key = cv::waitKey(1);
                if (key == 27) { // ESC key
                    stop();
                    break;
                }
            }


        }
    }
    
    IPCameraCapture m_camera;
    SegmentationClient m_segmentationClient;
    
    std::atomic<bool> m_isRunning;
    std::thread m_processingThread;
    
    // Frame queue
    std::queue<cv::Mat> m_frameQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    size_t m_processingQueueSize;
    
    // Visualization flag
    bool m_showVisualization;
};

int main(int argc, char* argv[]) {
    // Default camera URL (can be changed via command line)
    std::string cameraUrl = "http://10.10.3.72:8080/video";
    
    // Default server URL
    std::string serverUrl = "http://192.248.10.70:8000/segment";
    
    // Parse command line arguments
    if (argc > 1) {
        cameraUrl = argv[1];
    }
    
    std::cout << "Starting segmentation pipeline with camera: " << cameraUrl << std::endl;
    
    // Create and start the pipeline
    SegmentationPipeline pipeline(cameraUrl, serverUrl);
    if (!pipeline.start()) {
        std::cerr << "Failed to start the segmentation pipeline" << std::endl;
        return 1;
    }
    
    // Wait for user input to quit
    std::cout << "Press Enter to quit..." << std::endl;
    std::cin.get();
    
    // Stop the pipeline
    pipeline.stop();
    
    return 0;
}