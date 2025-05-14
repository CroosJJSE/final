#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

class IPCameraCapture {
public:
    // Callback type for new frame processing
    using FrameCallback = std::function<void(const cv::Mat&)>;
    
    IPCameraCapture(const std::string& cameraUrl);
    ~IPCameraCapture();
    
    // Start capturing frames (non-blocking)
    bool start();
    
    // Stop capturing frames
    void stop();
    
    // Check if capture is running
    bool isRunning() const;
    
    // Get the latest frame (blocking if no frame is available)
    cv::Mat getLatestFrame();
    
    // Set callback to be called when a new frame is captured
    void setFrameCallback(FrameCallback callback);
    
    // Set desired frame resolution
    void setResolution(int width, int height);

private:
    std::string m_cameraUrl;
    cv::VideoCapture m_capture;
    
    std::atomic<bool> m_isRunning;
    std::thread m_captureThread;
    
    // Frame storage
    cv::Mat m_latestFrame;
    std::mutex m_frameMutex;
    std::condition_variable m_frameCondition;
    bool m_newFrameAvailable;
    
    // Resolution settings
    int m_width;
    int m_height;
    
    // Callback for new frames
    FrameCallback m_frameCallback;
    std::mutex m_callbackMutex;
    
    // Thread function
    void captureLoop();
};