#!/bin/bash

RECODEX_DIR=/home/petr/Documents/MFF/ReCodEx
WORKER_REPO=${RECODEX_DIR}/worker
BROKER_REPO=${RECODEX_DIR}/broker

BROKER_EXEC=${BROKER_REPO}/build/recodex-broker
BROKER_CONF=${BROKER_REPO}/examples/config.yml
WORKER_EXEC=${WORKER_REPO}/build/recodex-worker
WORKER_CONF=${WORKER_REPO}/examples/config.yml

FILE_SERVER=${BROKER_REPO}/tools/file_server.py
FAKE_SUBMIT=${BROKER_REPO}/tools/fake_submit.py
HIPPOES_DIR=${RECODEX_DIR}/hrosi-ohradka

RESULT_DIR=~/Desktop/job_results


function start_broker {
	${BROKER_EXEC} -c ${BROKER_CONF} &
	BROKER_PID=$!
	echo "Broker started"
}

function stop_broker {
	kill ${BROKER_PID}
	wait ${BROKER_PID} 2> /dev/null
	echo "Broker stopped"
}

function start_worker {
	${WORKER_EXEC} -c ${WORKER_CONF} > /dev/null 2>&1 &
	WORKER_PID=$!
	echo "Worker started"
}

function stop_worker {
	kill ${WORKER_PID}
	wait ${WORKER_PID} 2> /dev/null
	echo "Worker stopped"
}


function start_file_server {
	TEMP_OUT=/tmp/file_server_output.txt
	${FILE_SERVER} > ${TEMP_OUT} 2>&1 &
	FILE_SERVER_PID=$!
	sleep 1
	SERVER_DIR=$(head -n 1 ${TEMP_OUT} | grep -o "/tmp/tmp[^ ]*")
	cp ${HIPPOES_DIR}/*.in ${HIPPOES_DIR}/*.out ${SERVER_DIR}/tasks
	echo "File server started in ${SERVER_DIR}"
}

function stop_file_server {
	kill ${FILE_SERVER_PID}
	wait ${FILE_SERVER_PID} 2> /dev/null
	rm ${TEMP_OUT}
	echo "File server stopped"
}

function prepare_submission {
	SUBM_DIR=/tmp/subm_hippoes
	mkdir -p ${SUBM_DIR}
	cp ${HIPPOES_DIR}/solution.c ${SUBM_DIR}
	cp ${WORKER_REPO}/examples/job-config-hippoes.yml ${SUBM_DIR}
	mv ${SUBM_DIR}/job-config-hippoes.yml ${SUBM_DIR}/job-config.yml
}

function cleanup_submission {
	rm -r ${SUBM_DIR}
}

function submit_hippoes {
	${FAKE_SUBMIT} ${SUBM_DIR} > /dev/null
	echo "Submitting job..."
}


# Prepare all resources
start_file_server
start_broker
start_worker
prepare_submission

# Send some submits
submit_hippoes
submit_hippoes

# Wait to finish execution
sleep 10

# Copy interesting files to result dir
cp -R ${SERVER_DIR}/results ${RESULT_DIR}

# Do the cleanup
cleanup_submission
stop_worker
stop_broker
stop_file_server

# Make accessible logs fron result dir
ln -s /var/log/recodex/broker.log ${RESULT_DIR}/broker.log
ln -s /var/log/recodex/worker.log ${RESULT_DIR}/worker.log

