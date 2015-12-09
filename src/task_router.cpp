#include "task_router.hpp"

void task_router::add_worker (worker worker)
{
	workers.push_back(worker);
}

worker::worker ()
{

}
