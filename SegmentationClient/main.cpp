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
        // Create output directory
        system("rm -rf ./output_frames");
        system("mkdir -p output_frames");
        
        int frame_count = 0;
        while (m_isRunning) {
            cv::Mat frame;
            
            // Get frame from queue (existing code remains same)
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_queueCondition.wait(lock, [this] { 
                    return !m_frameQueue.empty() || !m_isRunning; 
                });
                
                if (!m_isRunning) break;
                if (m_frameQueue.empty()) continue;
                
                frame = m_frameQueue.front();
                m_frameQueue.pop();
            }
            
            // Convert to grayscale
            cv::Mat grayFrame;
            cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);
            
            // Process segmentation
            auto start = std::chrono::high_resolution_clock::now();
            cv::Mat mask = m_segmentationClient.segmentImage(grayFrame);
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - start).count();
            
            if (!mask.empty()) {
                // Resize and process mask
                if (mask.size() != grayFrame.size()) {
                    cv::resize(mask, mask, grayFrame.size(), 0, 0, cv::INTER_NEAREST);
                }
                
                if (mask.channels() != 1) {
                    cv::cvtColor(mask, mask, cv::COLOR_BGR2GRAY);
                }
                
                cv::threshold(mask, mask, 1, 255, cv::THRESH_BINARY);
    
                // Create side-by-side result
                cv::Mat result;
                cv::cvtColor(grayFrame, result, cv::COLOR_GRAY2BGR);
                cv::Mat colorMask(mask.size(), CV_8UC3, cv::Scalar(0, 255, 0));
                colorMask.copyTo(result, mask);
                
                // Save all frames
                std::string frame_num = std::to_string(frame_count++);
                cv::imwrite("output_frames/original_" + frame_num + ".jpg", grayFrame);
                cv::imwrite("output_frames/mask_" + frame_num + ".jpg", mask);
                cv::imwrite("output_frames/result_" + frame_num + ".jpg", result);
                
                std::cout << "Saved frame " << frame_num 
                          << " | Processing time: " << duration << "ms" << std::endl;
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