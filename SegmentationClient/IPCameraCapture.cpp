#include "IPCameraCapture.h"
#include <iostream>

IPCameraCapture::IPCameraCapture(const std::string& cameraUrl)
    : m_cameraUrl(cameraUrl),
      m_isRunning(false),
      m_newFrameAvailable(false),
      m_width(600),
      m_height(350) {
}

IPCameraCapture::~IPCameraCapture() {
    stop();
}

bool IPCameraCapture::start() {
    if (m_isRunning) {
        return true; // Already running
    }
    
    // Open the camera
    if (!m_capture.open(m_cameraUrl)) {
        std::cerr << "Error: Could not open IP camera at URL: " << m_cameraUrl << std::endl;
        return false;
    }
    
    // Set resolution
    m_capture.set(cv::CAP_PROP_FRAME_WIDTH, m_width);
    m_capture.set(cv::CAP_PROP_FRAME_HEIGHT, m_height);
    
    // Check if camera is open
    if (!m_capture.isOpened()) {
        std::cerr << "Error: Failed to open camera after setting resolution." << std::endl;
        return false;
    }
    
    // Start the capture thread
    m_isRunning = true;
    m_captureThread = std::thread(&IPCameraCapture::captureLoop, this);
    
    return true;
}

void IPCameraCapture::stop() {
    if (!m_isRunning) {
        return;
    }
    
    // Signal the thread to stop
    m_isRunning = false;
    
    // Wait for the thread to finish
    if (m_captureThread.joinable()) {
        m_captureThread.join();
    }
    
    // Release the camera
    m_capture.release();
    
    // Notify any waiting threads
    {
        std::lock_guard<std::mutex> lock(m_frameMutex);
        m_newFrameAvailable = true;
    }
    m_frameCondition.notify_all();
}

bool IPCameraCapture::isRunning() const {
    return m_isRunning;
}

cv::Mat IPCameraCapture::getLatestFrame() {
    std::unique_lock<std::mutex> lock(m_frameMutex);
    
    // Wait until a new frame is available
    m_frameCondition.wait(lock, [this] { return m_newFrameAvailable || !m_isRunning; });
    
    // If stopped, return an empty frame
    if (!m_isRunning) {
        return cv::Mat();
    }
    
    // Reset the flag
    m_newFrameAvailable = false;
    
    // Return a copy of the latest frame
    return m_latestFrame.clone();
}

void IPCameraCapture::setFrameCallback(FrameCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_frameCallback = callback;
}

void IPCameraCapture::setResolution(int width, int height) {
    m_width = width;
    m_height = height;
    
    // If already running, apply the new resolution
    if (m_isRunning && m_capture.isOpened()) {
        m_capture.set(cv::CAP_PROP_FRAME_WIDTH, width);
        m_capture.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    }
}

void IPCameraCapture::captureLoop() {
    cv::Mat frame;
    
    while (m_isRunning) {
        // Capture a new frame
        if (!m_capture.read(frame)) {
            std::cerr << "Error: Failed to read frame from camera." << std::endl;
            
            // Try to reconnect
            m_capture.release();
            if (!m_capture.open(m_cameraUrl)) {
                std::cerr << "Error: Could not reconnect to camera. Retrying in 5 seconds..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            
            // Set resolution again
            m_capture.set(cv::CAP_PROP_FRAME_WIDTH, m_width);
            m_capture.set(cv::CAP_PROP_FRAME_HEIGHT, m_height);
            
            continue;
        }
        
        // Process the new frame
        if (!frame.empty()) {
            // Store the frame
            {
                std::lock_guard<std::mutex> lock(m_frameMutex);
                m_latestFrame = frame.clone();
                m_newFrameAvailable = true;
            }
            
            // Notify waiting threads
            m_frameCondition.notify_all();
            
            // Call the frame callback if set
            {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                if (m_frameCallback) {
                    m_frameCallback(frame);
                }
            }
        }
        
        // Small delay to avoid hogging CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(30)); // ~30 fps
    }
}