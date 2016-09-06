/*
 * Header file which serves to only purpose,
 *   it's a junkyard of mocked classes which can be used in test cases.
 */

#ifndef RECODEX_BROKER_TESTS_MOCKS_H
#define RECODEX_BROKER_TESTS_MOCKS_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../src/broker_connect.h"
#include "../src/worker.h"

using namespace testing;

class mock_broker_config : public broker_config
{
public:
	const std::string address = "*";
	const std::string localhost = "127.0.0.1";

	mock_broker_config() : broker_config()
	{
		ON_CALL(*this, get_worker_ping_interval()).WillByDefault(Return(std::chrono::milliseconds(1000)));

		ON_CALL(*this, get_client_address()).WillByDefault(ReturnRef(address));

		ON_CALL(*this, get_client_port()).WillByDefault(Return(1234));

		ON_CALL(*this, get_worker_address()).WillByDefault(ReturnRef(address));

		ON_CALL(*this, get_worker_port()).WillByDefault(Return(4321));

		ON_CALL(*this, get_monitor_address()).WillByDefault(ReturnRef(localhost));

		ON_CALL(*this, get_monitor_port()).WillByDefault(Return(7894));
	}

	MOCK_CONST_METHOD0(get_client_address, const std::string &());
	MOCK_CONST_METHOD0(get_client_port, uint16_t());
	MOCK_CONST_METHOD0(get_worker_address, const std::string &());
	MOCK_CONST_METHOD0(get_worker_port, uint16_t());
	MOCK_CONST_METHOD0(get_monitor_address, const std::string &());
	MOCK_CONST_METHOD0(get_monitor_port, uint16_t());
	MOCK_CONST_METHOD0(get_worker_ping_interval, std::chrono::milliseconds());
};

class mock_worker_registry : public worker_registry
{
public:
	MOCK_METHOD1(add_worker, void(worker_registry::worker_ptr));
	MOCK_METHOD1(remove_worker, void(worker_registry::worker_ptr));
	MOCK_METHOD1(deprioritize_worker, void(worker_registry::worker_ptr));
	MOCK_METHOD1(find_worker, worker_registry::worker_ptr(const request::headers_t &));
	MOCK_METHOD1(find_worker_by_identity, worker_registry::worker_ptr(const std::string &));
	MOCK_CONST_METHOD0(get_workers, const std::vector<std::shared_ptr<worker>> &());
};

class mock_connection_proxy
{
public:
	MOCK_METHOD3(set_addresses, void(const std::string &, const std::string &, const std::string &));
	MOCK_METHOD4(poll, void(message_origin::set &, std::chrono::milliseconds, bool &, std::chrono::milliseconds &));
	MOCK_METHOD3(recv_workers, bool(std::string &, std::vector<std::string> &, bool *));
	MOCK_METHOD3(recv_clients, bool(std::string &, std::vector<std::string> &, bool *));
	MOCK_METHOD2(send_workers, bool(const std::string &, const std::vector<std::string> &));
	MOCK_METHOD2(send_clients, bool(const std::string &, const std::vector<std::string> &));
	MOCK_METHOD1(send_monitor, bool(const std::vector<std::string> &));
};

class mock_status_notifier : public status_notifier_interface
{
public:
	MOCK_METHOD1(error, void(const std::string &));
	MOCK_METHOD2(rejected_job, void(const std::string &, const std::string &));
	MOCK_METHOD2(rejected_jobs, void(std::vector<std::string>, const std::string &));
	MOCK_METHOD1(job_done, void(const std::string &));
	MOCK_METHOD2(job_failed, void(const std::string &, const std::string &));
};

class mock_worker : public worker
{
public:
	mock_worker(
		const std::string &id, const std::string &hwgroup, const std::multimap<std::string, std::string> &headers)
		: worker(id, hwgroup, headers)
	{
	}

	MOCK_METHOD0(complete_request, void());
	MOCK_METHOD0(next_request, bool());
	MOCK_CONST_METHOD0(get_current_request, std::shared_ptr<const request>());
	MOCK_METHOD0(terminate, std::shared_ptr<std::vector<request_ptr>>());
};

#endif // RECODEX_WORKER_TESTS_MOCKS_H
