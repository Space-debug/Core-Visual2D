#include "Visual2DInfer.h"

#include <cv_bridge/cv_bridge.h>
#include <opencv2/imgproc.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>

#include "visual2d_infer/msg/detection2_d_array.hpp"
#include "visual2d_infer/msg/segment_result.hpp"

#include <algorithm>
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

namespace {

cv::Mat toBgr8(const sensor_msgs::msg::Image::ConstSharedPtr& msg)
{
    if (msg->encoding == sensor_msgs::image_encodings::BGR8) {
        return cv_bridge::toCvShare(msg, sensor_msgs::image_encodings::BGR8)->image;
    }
    if (msg->encoding == sensor_msgs::image_encodings::RGB8) {
        auto rgb = cv_bridge::toCvShare(msg, sensor_msgs::image_encodings::RGB8)->image;
        cv::Mat bgr;
        cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);
        return bgr;
    }
    if (msg->encoding == sensor_msgs::image_encodings::MONO8) {
        auto gray = cv_bridge::toCvShare(msg, sensor_msgs::image_encodings::MONO8)->image;
        cv::Mat bgr;
        cv::cvtColor(gray, bgr, cv::COLOR_GRAY2BGR);
        return bgr;
    }

    auto converted = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
    return converted->image;
}

visual2d_infer::msg::Detection2DArray toDetectionMsg(
    const std_msgs::msg::Header& header,
    int64_t frame_id,
    const trtyolo::DetectRes& detect_res)
{
    visual2d_infer::msg::Detection2DArray msg;
    msg.header = header;
    msg.frame_id = frame_id;
    msg.detections.reserve(detect_res.boxes.size());

    for (size_t i = 0; i < detect_res.boxes.size(); ++i) {
        visual2d_infer::msg::Detection2D det;
        det.class_id = (i < detect_res.classes.size()) ? detect_res.classes[i] : -1;
        det.score = (i < detect_res.scores.size()) ? detect_res.scores[i] : 0.0f;
        det.left = detect_res.boxes[i].left;
        det.top = detect_res.boxes[i].top;
        det.right = detect_res.boxes[i].right;
        det.bottom = detect_res.boxes[i].bottom;
        msg.detections.push_back(det);
    }
    return msg;
}

visual2d_infer::msg::SegmentResult toSegmentMsg(
    const std_msgs::msg::Header& header,
    int64_t frame_id,
    const trtyolo::SegmentRes& segment_res)
{
    visual2d_infer::msg::SegmentResult msg;
    msg.header = header;
    msg.frame_id = frame_id;
    msg.instances.reserve(segment_res.boxes.size());

    for (size_t i = 0; i < segment_res.boxes.size(); ++i) {
        visual2d_infer::msg::SegmentInstance inst;
        inst.class_id = (i < segment_res.classes.size()) ? segment_res.classes[i] : -1;
        inst.score = (i < segment_res.scores.size()) ? segment_res.scores[i] : 0.0f;
        inst.left = segment_res.boxes[i].left;
        inst.top = segment_res.boxes[i].top;
        inst.right = segment_res.boxes[i].right;
        inst.bottom = segment_res.boxes[i].bottom;

        if (i < segment_res.masks.size()) {
            const auto& mask = segment_res.masks[i];
            inst.mask_width = static_cast<uint32_t>(mask.width);
            inst.mask_height = static_cast<uint32_t>(mask.height);
            inst.mask_data = mask.data;
        }
        msg.instances.push_back(inst);
    }
    return msg;
}

}  // namespace

class Visual2DNode : public rclcpp::Node {
public:
    Visual2DNode()
        : Node("visual2d_infer")
    {
        engine_path_ = declare_parameter<std::string>("engine_path", "");
        image_topic_ = declare_parameter<std::string>("image_topic", "image_raw");
        thread_num_ = declare_parameter<int>("thread_num", 1);

        if (engine_path_.empty()) {
            throw std::runtime_error("Parameter 'engine_path' is required");
        }

        const int max_thread_num = std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
        thread_num_ = std::clamp(thread_num_, 1, max_thread_num);

        RCLCPP_INFO(get_logger(), "Loading engine: %s", engine_path_.c_str());
        RCLCPP_INFO(get_logger(), "Using %d inference thread(s)", thread_num_);

        infer_ = std::make_unique<Visual2DInfer>(engine_path_, static_cast<uint8_t>(thread_num_));

        if (infer_->task_type_ == TaskType::DETECT) {
            detect_pub_ = create_publisher<visual2d_infer::msg::Detection2DArray>("detections", 10);
        } else if (infer_->task_type_ == TaskType::SEGMENT) {
            segment_pub_ = create_publisher<visual2d_infer::msg::SegmentResult>("segments", 10);
        }

        image_sub_ = create_subscription<sensor_msgs::msg::Image>(
            image_topic_, rclcpp::SensorDataQoS(),
            std::bind(&Visual2DNode::onImage, this, std::placeholders::_1));

        result_thread_ = std::thread(&Visual2DNode::resultLoop, this);

        RCLCPP_INFO(get_logger(), "Subscribed to '%s'", image_topic_.c_str());
    }

    ~Visual2DNode() override
    {
        running_.store(false);
        if (infer_) {
            infer_->stop_.store(true);
        }
        if (result_thread_.joinable()) {
            result_thread_.join();
        }
    }

private:
    void onImage(const sensor_msgs::msg::Image::ConstSharedPtr& msg)
    {
        if (!running_.load()) {
            return;
        }

        cv::Mat bgr;
        try {
            bgr = toBgr8(msg);
        } catch (const cv_bridge::Exception& e) {
            RCLCPP_WARN(get_logger(), "cv_bridge conversion failed: %s", e.what());
            return;
        }

        if (bgr.empty()) {
            RCLCPP_WARN(get_logger(), "Received empty image");
            return;
        }

        const int64_t frame_id = next_submit_id_.fetch_add(1);
        cv::Mat image_mat = bgr.clone();

        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            pending_headers_[frame_id] = msg->header;
            pending_images_[frame_id] = std::move(image_mat);
        }

        cv::Mat stored;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            stored = pending_images_.at(frame_id);
        }
        trtyolo::Image image(
            stored.data, stored.cols, stored.rows, stored.channels(),
            static_cast<size_t>(stored.step));
        infer_->submitTask(image, frame_id);
    }

    void resultLoop()
    {
        while (running_.load() && rclcpp::ok()) {
            UnifiedResult result;
            const int64_t frame_id = next_result_id_.load();

            if (!infer_->getResult(result, frame_id)) {
                break;
            }

            std_msgs::msg::Header header;
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                auto header_it = pending_headers_.find(frame_id);
                if (header_it != pending_headers_.end()) {
                    header = header_it->second;
                    pending_headers_.erase(header_it);
                }
                pending_images_.erase(frame_id);
            }

            if (result.isDetect() && detect_pub_) {
                auto msg = toDetectionMsg(header, frame_id, result.asDetect());
                detect_pub_->publish(msg);
                RCLCPP_DEBUG(get_logger(), "Published %zu detections for frame %ld",
                             msg.detections.size(), frame_id);
            } else if (result.isSegment() && segment_pub_) {
                auto msg = toSegmentMsg(header, frame_id, result.asSegment());
                segment_pub_->publish(msg);
                RCLCPP_DEBUG(get_logger(), "Published %zu segment instances for frame %ld",
                             msg.instances.size(), frame_id);
            }

            next_result_id_.fetch_add(1);
        }
    }

    std::string engine_path_;
    std::string image_topic_;
    int thread_num_{1};

    std::unique_ptr<Visual2DInfer> infer_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
    rclcpp::Publisher<visual2d_infer::msg::Detection2DArray>::SharedPtr detect_pub_;
    rclcpp::Publisher<visual2d_infer::msg::SegmentResult>::SharedPtr segment_pub_;

    std::atomic<bool> running_{true};
    std::atomic<int64_t> next_submit_id_{0};
    std::atomic<int64_t> next_result_id_{0};

    std::mutex state_mutex_;
    std::unordered_map<int64_t, std_msgs::msg::Header> pending_headers_;
    std::unordered_map<int64_t, cv::Mat> pending_images_;

    std::thread result_thread_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    try {
        auto node = std::make_shared<Visual2DNode>();
        rclcpp::spin(node);
    } catch (const std::exception& e) {
        RCLCPP_ERROR(rclcpp::get_logger("visual2d_infer"), "Fatal error: %s", e.what());
        rclcpp::shutdown();
        return 1;
    }
    rclcpp::shutdown();
    return 0;
}
