#include "convert.h"


// 构造函数：根据CUDA启用状态初始化映射图
ImageConvert::ImageConvert(int input_w, int input_h, int output_w, int output_h, float fov)
    : input_width_(input_w), input_height_(input_h),
      output_width_(output_w), output_height_(output_h), 
      fov_(fov) 
{
    for (int i = 0; i < 4; ++i) {
        cv::Mat cpu_map_x, cpu_map_y;
        ComputeRemap(i * 90, 0, cpu_map_x, cpu_map_y);  // 获得remap矩阵
        view_maps_[i] = {cpu_map_x, cpu_map_y};
    }
}


void ImageConvert::ComputeRemap(float theta, float phi, cv::Mat &map_x, cv::Mat &map_y) const {
    int Wd = output_width_;
    int Hd = output_height_;
    map_x = cv::Mat::zeros(Hd, Wd, CV_32FC1);
    map_y = cv::Mat::zeros(Hd, Wd, CV_32FC1);

    float wFOV = fov_;
    float hFOV = Hd * 1.0 / Wd * wFOV;
    float w_len = 2 * std::tan(wFOV * CV_PI / 360.0);
    float h_len = 2 * std::tan(hFOV * CV_PI / 360.0);
    cv::Mat rotation_matrix = ComputeRotationMatrix(theta, phi);

    // 使用OpenMP并行化双重循环（需编译器支持OpenMP）
    #pragma omp parallel for collapse(2)  // 合并x和y循环为一层并行
    for (int x = 0; x < Wd; ++x) {
        for (int y = 0; y < Hd; ++y) {
            float x_ = 1;
            float y_ = (x * 1.0 / Wd - 0.5) * w_len;
            float z_ = (y * 1.0 / Hd - 0.5) * h_len;
            float r = std::sqrt(x_ * x_ + y_ * y_ + z_ * z_);
            x_ /= r; y_ /= r; z_ /= r;

            cv::Mat XYZ = (cv::Mat_<double>(3,1) << x_, y_, z_);
            XYZ = rotation_matrix * XYZ;

            float lon = (std::asin(XYZ.at<double>(2)) / CV_PI + 0.5) * input_height_;
            float lat = (std::atan2(XYZ.at<double>(1), XYZ.at<double>(0)) / (2 * CV_PI) + 0.5) * input_width_;

            map_x.at<float>(y, x) = lat;
            map_y.at<float>(y, x) = lon;
        }
    }
}


// GetViews函数：根据CUDA启用状态选择计算路径
std::vector<cv::Mat> ImageConvert::GetViews(cv::Mat &frame) 
{
    std::vector<cv::Mat> images(4);

    // #pragma omp parallel for num_threads(4)  // 启动4线程
    #pragma omp parallel for num_threads(4) schedule(static)
    for (int i = 0; i < 4; ++i) {
        auto &view = view_maps_[i];
        cv::Mat view_image;
        cv::remap(
            frame, view_image,
            view.first, view.second,
            cv::INTER_CUBIC, cv::BORDER_CONSTANT
        );

        images[i] = view_image.clone();
    }

    return images;
}


cv::Mat ImageConvert::ComputeRotationMatrix(float theta, float phi) const {
    float alpha = 0;
    float beta = -phi * CV_PI / 180.0;
    float gamma = theta * CV_PI / 180.0;

    float cos_alpha = std::cos(alpha);
    float sin_alpha = std::sin(alpha);
    float cos_beta  = std::cos(beta);
    float sin_beta  = std::sin(beta);
    float cos_gamma = std::cos(gamma);
    float sin_gamma = std::sin(gamma); 

    cv::Mat Rx = (cv::Mat_<double>(3, 3) <<
            1, 0, 0,
            0, cos_alpha, -sin_alpha,
            0, sin_alpha, cos_alpha
    );

    cv::Mat Ry = (cv::Mat_<double>(3, 3) <<
            cos_beta, 0, -sin_alpha,
            0, 1, 0,
            sin_alpha, 0, cos_beta
    );

    cv::Mat Rz = (cv::Mat_<double>(3, 3) <<
            cos_gamma, -sin_gamma, 0,
            sin_gamma, cos_gamma, 0,
            0, 0, 1
    );

    return Rz * Ry * Rx;        // 3*3的，计算速度较快
}