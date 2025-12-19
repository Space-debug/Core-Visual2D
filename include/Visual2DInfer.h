#pragma once

#include <algorithm>
#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <atomic>
#include <unordered_map>
#include <condition_variable>
#include <variant>
#include <memory>

#include "deploy/trtyolo.hpp"


enum class TaskType {
    DETECT,
    SEGMENT,
    // ...
};

struct UnifiedResult {
    TaskType type;
    std::variant<trtyolo::DetectRes, trtyolo::SegmentRes> data; // 暂不包含 Pose

    bool isDetect() const { return type == TaskType::DETECT; }
    bool isSegment() const { return type == TaskType::SEGMENT; }

    trtyolo::DetectRes& asDetect() { return std::get<trtyolo::DetectRes>(data); }
    trtyolo::SegmentRes& asSegment() { return std::get<trtyolo::SegmentRes>(data); }
};


class Visual2DInfer {
public:
    static constexpr int BUFFER_SIZE = 10;
    std::atomic<bool> stop_{false};
    TaskType task_type_;

    ~Visual2DInfer();
    Visual2DInfer(const std::string& engine_file, uint8_t thread_num);

    void submitTask(const trtyolo::Image& img, int64_t id);
    bool getResult(UnifiedResult& result, int64_t id);

private:
    void inference(size_t id);
    static TaskType inferTaskType(const std::string& engine_file);

    std::vector<std::variant<
        std::unique_ptr<trtyolo::DetectModel>,
        std::unique_ptr<trtyolo::SegmentModel>
    >> models_;

    std::mutex mtx_tasks_;
    std::condition_variable cv_task_;
    std::queue<std::pair<int64_t, trtyolo::Image>> tasks_;

    std::mutex mtx_results_;
    std::condition_variable cv_result_;
    std::unordered_map<int64_t, UnifiedResult> results_;

    std::vector<std::thread> threads_;
};