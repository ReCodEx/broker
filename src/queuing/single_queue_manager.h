#ifndef RECODEX_BROKER_SINGLE_QUEUE_MANAGER_HPP
#define RECODEX_BROKER_SINGLE_QUEUE_MANAGER_HPP

#include "queue_manager_interface.h"

#include <chrono>
#include <memory>
#include <vector>
#include <map>
#include <algorithm>

struct request_entry {
    request_ptr request;
    std::chrono::milliseconds arrived_at;
};

struct fcfs_job_comparator {
    bool compare(const request_entry &a, const request_entry &b, worker_ptr worker) const
    {
        return a.arrived_at < b.arrived_at;
    }

    void drop_caches() {}
};


struct first_idle_worker_selector {
    worker_ptr select(const std::map<worker_ptr, request_ptr> &worker_jobs, const std::vector<request_entry> &queued_jobs, request_ptr request) const
    {
        for (auto &pair: worker_jobs) {
            if (pair.second == nullptr && pair.first->check_headers(request->headers)) {
                return pair.first;
            }
        }

        return nullptr;
    }
};

template <typename JobComparator = fcfs_job_comparator, typename IdleWorkerSelector = first_idle_worker_selector>
class single_queue_manager : public queue_manager_interface
{
private:
    std::unique_ptr<JobComparator> comparator_;
    std::unique_ptr<IdleWorkerSelector> selector_;
    std::vector<request_entry> jobs_;
    std::map<worker_ptr, request_ptr> worker_jobs_;
    std::vector<worker_ptr> workers_;

public:
    explicit single_queue_manager():
        comparator_(std::make_unique<JobComparator>()), selector_(std::make_unique<IdleWorkerSelector>())
    {}

    explicit single_queue_manager(std::unique_ptr<JobComparator> comparator):
        comparator_(std::move(comparator)), selector_(std::make_unique<IdleWorkerSelector>())
    {}

    single_queue_manager(std::unique_ptr<JobComparator> comparator, std::unique_ptr<IdleWorkerSelector> selector):
        comparator_(std::move(comparator)), selector_(std::move(selector))
    {}

    ~single_queue_manager() override = default;

    request_ptr add_worker(worker_ptr worker, request_ptr current_request = nullptr) override
    {
        worker_jobs_[worker] = current_request;
        workers_.push_back(worker);

        if (current_request != nullptr) {
            return current_request;
        }

        return assign_request(worker);
    }

    request_ptr assign_request(worker_ptr worker) override
    {
        worker_jobs_[worker] = nullptr;
        comparator_->drop_caches();

        std::sort(jobs_.begin(), jobs_.end(), [this, worker] (const request_entry &a, const request_entry &b) {
            return comparator_->compare(a, b, worker);
        });

        for (auto it = jobs_.cbegin(); it != jobs_.cend(); ++it) {
            if (!worker->check_headers(it->request->headers)) {
                continue;
            }

            worker_jobs_[worker] = it->request;
            jobs_.erase(it);

            return worker_jobs_[worker];
        }

        return nullptr;
    }

    std::shared_ptr<std::vector<request_ptr>> worker_terminated(worker_ptr worker) override
    {
        auto result = std::make_shared<std::vector<request_ptr>>();
        result->push_back(worker_jobs_[worker]);
        worker_jobs_.erase(worker);
        workers_.erase(std::remove(workers_.begin(), workers_.end(), worker), workers_.end());
        return result;
    }

    enqueue_result enqueue_request(request_ptr request) override
    {
        // Try to find an idle worker and assign the job
        auto idle_worker = selector_->select(worker_jobs_, jobs_, request);
        if (idle_worker) {
            worker_jobs_[idle_worker] = request;

            return enqueue_result{
                .assigned_to = idle_worker,
                .enqueued = true,
            };
        }

        // If no worker able to process the job exists, reject it
        for (auto it = std::begin(worker_jobs_); it != std::end(worker_jobs_); ++it) {
            if (it->first->check_headers(request->headers)) {
                break;
            }

            if (std::next(it) == std::end(worker_jobs_)) {
                return enqueue_result{
                    .assigned_to = nullptr,
                    .enqueued = false,
                };
            }
        }

        // Enqueue the job
        jobs_.push_back(request_entry{
            .request = request,
            .arrived_at = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ),
        });

        return enqueue_result{
            .assigned_to = nullptr,
            .enqueued = true,
        };
    }

    std::size_t get_queued_request_count() override
    {
        return jobs_.size();
    }

    request_ptr get_current_request(worker_ptr worker) override
    {
        return worker_jobs_[worker];
    }

    request_ptr worker_finished(worker_ptr worker) override
    {
        return assign_request(worker);
    }

    request_ptr worker_cancelled(worker_ptr worker) override
    {
        auto current_request = worker_jobs_[worker];
        worker_jobs_[worker] = nullptr;
        return current_request;
    }
};

#endif
