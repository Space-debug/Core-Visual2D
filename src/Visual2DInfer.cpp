#include "Visual2DInfer.h"

#include <cctype>
#include <chrono>

// === 辅助：从引擎文件名推断任务类型 ===
TaskType Visual2DInfer::inferTaskType(const std::string& engine_file) {
    std::string lower = engine_file;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (lower.find("seg") != std::string::npos ||
        lower.find("mask") != std::string::npos) {
        return TaskType::SEGMENT;
    }

    // 可扩展：if (lower.find("pose") ...) return TaskType::POSE;
    return TaskType::DETECT;
}

// === 构造函数 ===
Visual2DInfer::Visual2DInfer(const std::string& engine_file, uint8_t thread_num)
    : task_type_(inferTaskType(engine_file)), stop_(false)
{
    auto option = trtyolo::InferOption();
    option.enableSwapRB();
    option.enablePerformanceReport();

    models_.reserve(thread_num);
    if (task_type_ == TaskType::DETECT) {
        auto base_model = std::make_unique<trtyolo::DetectModel>(engine_file, option);
        for (uint8_t i = 0; i < thread_num; ++i) {
            models_.emplace_back(base_model->clone());
        }
    } else if (task_type_ == TaskType::SEGMENT) {
        auto base_model = std::make_unique<trtyolo::SegmentModel>(engine_file, option);
        for (uint8_t i = 0; i < thread_num; ++i) {
            models_.emplace_back(base_model->clone());
        }
    } else {
        // ...
    }

    threads_.reserve(thread_num);
    for (size_t i = 0; i < thread_num; ++i) {
        threads_.emplace_back(&Visual2DInfer::inference, this, i);
    }
}

// === 析构函数 ===
Visual2DInfer::~Visual2DInfer() {
    {
        std::lock_guard<std::mutex> lock(mtx_tasks_);
        stop_ = true;
    }
    cv_task_.notify_all();

    for (auto& t : threads_) {
        if (t.joinable()) t.join();
    }

    std::cout << "Visual2DInfer destructor" << std::endl;
}

// === 推理线程函数 ===
void Visual2DInfer::inference(size_t id) {
    while (true) {
        {
            std::unique_lock<std::mutex> lock(mtx_tasks_);
            cv_task_.wait(lock, [this] { return !tasks_.empty() || stop_.load(); });
            if (stop_ && tasks_.empty()) return;

            auto task = std::move(tasks_.front()); 
            tasks_.pop();
            lock.unlock(); // 提前释放锁，减少临界区

            // --- 推理逻辑 ---
            UnifiedResult unified;
            unified.type = task_type_;

            if (task_type_ == TaskType::DETECT) {
                auto& model = std::get<std::unique_ptr<trtyolo::DetectModel>>(models_[id]);
                unified.data = model->predict(task.second);
            } else if (task_type_ == TaskType::SEGMENT) {
                auto& model = std::get<std::unique_ptr<trtyolo::SegmentModel>>(models_[id]);
                unified.data = model->predict(task.second);
            }

            {
                std::lock_guard<std::mutex> lk(mtx_results_);
                results_.emplace(task.first, std::move(unified));
            }
            cv_result_.notify_one();
        }
    }
}

// === 提交任务 ===
void Visual2DInfer::submitTask(const trtyolo::Image& img, int64_t id) {
    while (tasks_.size() > BUFFER_SIZE && !threads_.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    {
        std::lock_guard<std::mutex> lock(mtx_tasks_);
        tasks_.emplace(id, img);
    }
    cv_task_.notify_one();
}

// === 获取结果 ===
bool Visual2DInfer::getResult(UnifiedResult& result, int64_t id) {
    std::unique_lock<std::mutex> lock(mtx_results_);
    cv_result_.wait(lock, [this, id] {
        return results_.find(id) != results_.end() || stop_.load();
    });

    if (stop_) return false;

    auto it = results_.find(id);
    if (it == results_.end()) return false;

    result = std::move(it->second);
    results_.erase(it);
    return true;
}