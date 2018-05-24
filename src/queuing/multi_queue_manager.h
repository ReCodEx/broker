#ifndef RECODEX_BROKER_MULTI_QUEUE_MANAGER_HPP
#define RECODEX_BROKER_MULTI_QUEUE_MANAGER_HPP

#include "queue_manager_interface.h"
#include <list>

/**
 * Manages a separate request queue for every worker
 */
class multi_queue_manager : public queue_manager_interface
{
private:
	std::map<worker_ptr, std::queue<request_ptr>> queues_;
	std::map<worker_ptr, request_ptr> current_requests_;
	std::list<worker_ptr> worker_queue_;

public:
	~multi_queue_manager() override = default;
	request_ptr add_worker(worker_ptr worker, request_ptr current_request = nullptr) override;
	request_ptr assign_request(worker_ptr worker) override;
	std::shared_ptr<std::vector<request_ptr>> worker_terminated(worker_ptr) override;
	enqueue_result enqueue_request(request_ptr request) override;
	size_t get_queued_request_count() override;
	request_ptr get_current_request(worker_ptr worker) override;
	request_ptr worker_finished(worker_ptr worker) override;
	request_ptr worker_cancelled(worker_ptr worker) override;
};


#endif // RECODEX_BROKER_MULTI_QUEUE_MANAGER_HPP
