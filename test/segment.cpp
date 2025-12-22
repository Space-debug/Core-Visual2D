#include "convert.h"
#include "Visual2DInfer.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <memory>

std::unique_ptr<Visual2DInfer> g_detector;

void getData(const std::string& input_video_path)
{
    cv::VideoCapture cap = cv::VideoCapture(input_video_path);
    if (!cap.isOpened()) {
        std::cerr << "Error: Cannot open video file: " << input_video_path << std::endl;
        return;
    }

    ImageConvert converter(5760, 2880, 640, 480, 120);

    cv::Mat frame;
    int64_t frame_id = 0;
    while (cap.isOpened() && !g_detector->stop_.load()) {
        if (!cap.read(frame) || frame.empty()) {
            break;
        }
        
        auto views = converter.GetViews(frame);
        for (const auto& view : views) {
            g_detector->submitTask(trtyolo::Image(view.data, view.cols, view.rows), frame_id++);
        }
    }
    
    std::cout << "Video processing completed. Total frames processed: " << frame_id << std::endl;
}


int main(int argc, char** argv) 
{
    std::string model_path           = "../../models/yolo11n-seg.engine";
    std::string input_video_path     = "../../test.mp4";
    int         requested_thread_num = 1;

    // Get system supported maximum thread count
    int max_thread_num = std::thread::hardware_concurrency();
    if (max_thread_num <= 0) {
        max_thread_num = 2;
    }

    // Limit thread count to system maximum
    int thread_num = std::min(requested_thread_num, max_thread_num);
    std::cout << "Using " << thread_num << " threads (max supported: " << max_thread_num << ")." << std::endl;

    // Create detector
    g_detector = std::make_unique<Visual2DInfer>(model_path, static_cast<uint8_t>(thread_num));

    // Start data acquisition thread
    std::thread data_thread(getData, input_video_path);

    // Get results - process 4 views at a time
    int64_t frame_id = 0;
    while (true) {
        bool all_results_received = true;
        
        for (int i = 0; i < 4; ++i) {
            UnifiedResult result;
            if (g_detector->getResult(result, frame_id)) {
                if (result.isSegment()) {
                    auto& segment_res = result.asSegment();
                    std::cout << "Frame " << frame_id << " - Segment Result:" << std::endl;
                    std::cout << "  Num: " << segment_res.num << std::endl;
                    std::cout << "  Classes: ";
                    for (size_t i = 0; i < segment_res.classes.size(); ++i) {
                        std::cout << segment_res.classes[i];
                        if (i < segment_res.classes.size() - 1) std::cout << ", ";
                    }
                    std::cout << std::endl;
                    std::cout << "  Scores: ";
                    for (size_t i = 0; i < segment_res.scores.size(); ++i) {
                        std::cout << segment_res.scores[i];
                        if (i < segment_res.scores.size() - 1) std::cout << ", ";
                    }
                    std::cout << std::endl;
                    std::cout << "  Boxes: " << segment_res.boxes.size() << std::endl;
                    for (size_t i = 0; i < segment_res.boxes.size() && i < 5; ++i) {
                        const auto& box = segment_res.boxes[i];
                        std::cout << "    [" << i << "] left=" << box.left << ", top=" << box.top 
                                  << ", right=" << box.right << ", bottom=" << box.bottom << std::endl;
                    }
                    if (segment_res.boxes.size() > 5) {
                        std::cout << "    ... (" << (segment_res.boxes.size() - 5) << " more boxes)" << std::endl;
                    }
                    std::cout << "  Masks: " << segment_res.masks.size() << std::endl;
                    for (size_t i = 0; i < segment_res.masks.size() && i < 3; ++i) {
                        const auto& mask = segment_res.masks[i];
                        std::cout << "    [" << i << "] size=" << mask.width << "x" << mask.height 
                                  << ", data_size=" << mask.data.size() << std::endl;
                    }
                    if (segment_res.masks.size() > 3) {
                        std::cout << "    ... (" << (segment_res.masks.size() - 3) << " more masks)" << std::endl;
                    }
                } else {
                    std::cout << "Frame " << frame_id << " - Unexpected result type" << std::endl;
                }
                frame_id++;
            } else {
                all_results_received = false;
                // If video processing is done and no more results, exit
                if (g_detector->stop_.load() && data_thread.joinable()) {
                    data_thread.join();
                    if (!g_detector->getResult(result, frame_id)) {
                        std::cout << "No more results available. Exiting..." << std::endl;
                        return 0;
                    }
                }
                break;
            }
        }
        
        if (all_results_received) {
            std::cout << "======================================" << std::endl;
        } else {
            // Wait a bit before checking again
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    if (data_thread.joinable()) {
        data_thread.join();
    }

    return 0;
}