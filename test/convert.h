#pragma once

#include <vector>
#include <array>
#include <omp.h>
#include <unordered_map>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>  


class ImageConvert {
private:
    float fov_;         // 120;
    int input_width_;   // 5760;
    int input_height_;  // 2880;
    int output_width_;  // 640;
    int output_height_; // 480;

    std::array<std::pair<cv::Mat, cv::Mat>, 4> view_maps_;  // CPU映射图
    void ComputeRemap(float theta, float phi, cv::Mat &map_x, cv::Mat &map_y) const;
    cv::Mat ComputeRotationMatrix(float theta, float phi) const;

public:
    ImageConvert(int input_w, int input_h, int output_w, int output_h, float fov);
    std::vector<cv::Mat> GetViews(cv::Mat &frame);
};