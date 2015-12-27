#!/usr/bin/env python3

# Send a fake submit to the ReCodEx broker
# usage:
#       $ python fake_submit.py --header1 val1 --header2 val2
# The script assumes that there is a config.yml file in its 
# working directory. If you don't want to supply your own file,
# run the script in the examples directory.

import zmq
import yaml
import sys
import requests

fsrv_port = 9999
headers = {}
files = []
argv_it = iter(sys.argv)

# Load arguments
for arg in argv_it:
    if arg.startswith("--"):
        headers[arg[2:]] = next(argv_it)
    else:
        files.append(arg)

# Send the submission to our fake file server
reply = requests.post(
    "http://localhost:{0}".format(fsrv_port),
    {f.encode(): open(f, "r").read() for f in files}
)
job_id = reply.text

# Sniff out the brokers port
with open("config.yml") as config_file:
    config = yaml.load(config_file)

# Connect to the broker
context = zmq.Context()
broker = context.socket(zmq.REQ)
broker.connect("tcp://localhost:{}".format(config['client_port']))

# Send the request
broker.send_multipart(
    [b"eval"] +
    [str(job_id).encode()] +
    ["{}={}".format(k, v).encode() for k, v in headers.items()] +
    [b""] +
    ["http://localhost:{0}/submission_archives/{1}.tar.gz".format(fsrv_port, job_id).encode()] +
    ["http://localhost:{0}/results/example/".format(fsrv_port).encode()]
)

result = broker.recv()
print(result)
