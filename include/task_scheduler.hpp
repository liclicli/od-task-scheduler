#include <queue>
#include <optional>
#include <shared_mutex>
#include <atomic>

namespace odts {
    template <typename T>
    class task_queue {
    public:
        task_queue(size_t n): top_edge_(n) {}

        bool to_zero() {
            std::shared_lock lock(mutex_);
            return to_zero_;
        }

        size_t size() {
            std::shared_lock lock(mutex_);
            return data_.size();
        }

        void push_back(const T &value) {
            std::unique_lock lock(mutex_);
            data_.push(value);
            if (data_.size() == top_edge_) {
                to_zero_ = false;
            }
        }

        void push_back(T &&value) {
            std::unique_lock lock(mutex_);
            data_.push(value);
            if (data_.size() == top_edge_) {
                to_zero_ = false;
            }
        }

        std::optional<T> pop_front() {
            std::unique_lock lock(mutex_);
            if (data_.size() > 0) {
                std::optional<T> value = data_.front();
                data_.pop();
                if (data_.size() == 0) {
                    to_zero_ = true;
                }
                return value;
            }
            return std::optional<T>();
        }

    private:
        std::shared_mutex mutex_;
        std::queue<T> data_;
        size_t top_edge_;
        volatile bool to_zero_ = true;
    };

    template <typename T>
    class task_executor {
    public:
        task_executor(size_t chain_size,
                    std::shared_ptr<task_queue<T>> queue,
                    std::function<std::function<void()>(T)> to_task,
                    std::function<void(std::function<void()>)> dispatch)
                    : flag_(std::make_shared<std::atomic_flag>(false)),
                    queue_(queue),
                    to_task_(to_task),
                    dispatch_(dispatch) {
            next_ = nullptr;
            if (chain_size - 1 > 0) {
                next_ = std::make_shared<task_executor>(chain_size - 1, queue, to_task, dispatch);
            }
        }

        void try_execute() {
            auto waiting_size = queue_->size();
            if (queue_->to_zero()) {
                try_execute(waiting_size);
            }
        }

        void try_execute(size_t waiting_size) {
            if (waiting_size == 0) {
                return ;
            }

            if (!flag_->test_and_set()) {
                execute();
                --waiting_size;
            }

            if (next_ != nullptr && waiting_size > 0) {
                next_->try_execute(waiting_size);
            }
        }

        void execute() {
            auto queue = queue_;
            auto to_task = to_task_;
            auto flag = flag_;
            dispatch_([queue, to_task, flag](){
                do {
                    auto front = queue->pop_front();
                    if (front) {
                        to_task(*front)();
                    }
                    if (queue->size() == 0) {
                        flag->clear();
                        if (queue->size() > 0 && !flag->test_and_set())
                        {
                            continue;
                        }
                        return;
                    }
                } while(true);
            });
        }

    private:
        std::shared_ptr<task_executor<T>> next_;
        std::shared_ptr<task_queue<T>> queue_;
        std::shared_ptr<std::atomic_flag> flag_;
        std::function<std::function<void()>(T)> to_task_;
        std::function<void(std::function<void()>)> dispatch_;
    };

    template <typename T>
    class task_scheduler {
    public:
        task_scheduler(size_t max_executor,
                    std::function<std::function<void()>(T)> to_task,
                    std::function<void(std::function<void()>)> dispatch)
                    : queue_(std::make_shared<task_queue<T>>(max_executor)) {
            executor_ = std::make_shared<task_executor<T>>(max_executor, queue_, to_task, dispatch);
        }
        void append(const T &data) {
            queue_->push_back(data);
            executor_->try_execute();
        }

        void append(T &&data) {
            queue_->push_back(data);
            executor_->try_execute();
        }

    private:
        std::shared_ptr<task_queue<T>> queue_;
        std::shared_ptr<task_executor<T>> executor_;
    };
}
