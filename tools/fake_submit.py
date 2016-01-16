#!/usr/bin/env python3

# Send a fake submit to the ReCodEx broker
# usage:
#       $ python fake_submit.py --header1 val1 --header2 val2 submit_directory

import zmq
import os
import sys
import requests

fsrv_port = 9999
broker_port = 9658

submit_dir = ""
headers = {}
argv_it = iter(sys.argv[1:])

for arg in argv_it:
    if arg.startswith("--") and len(arg) > 2:
        headers[arg[2:]] = next(argv_it)
    else:
        submit_dir = arg

if submit_dir == "":
    sys.exit("no directory to submit was specified")

# An iterator for the submitted files
def walk_submit ():
    for root, dirs, files in os.walk(submit_dir):
        for name in files:
            yield os.path.relpath(os.path.join(root, name), submit_dir)

# Send the submission to our fake file server
try:
    reply = requests.post(
        "http://localhost:{0}".format(fsrv_port),
        {
            f.encode(): open(os.path.join(submit_dir, f), "rb").read() 
            for f in walk_submit()
        }
    )
except:
    sys.exit("Error sending files to the file server")

job_id = reply.text

# Connect to the broker
context = zmq.Context()
broker = context.socket(zmq.REQ)

try:
    broker.connect("tcp://localhost:{}".format(broker_port))
except:
    sys.exit("Error connecting to the broker")

# Send the request
broker.send_multipart(
    [b"eval"] +
    [str(job_id).encode()] +
    ["{}={}".format(k, v).encode() for k, v in headers.items()] +
    [b""] +
    ["http://localhost:{0}/submit_archives/{1}.tar.gz".format(fsrv_port, job_id).encode()] +
    ["http://localhost:{0}/results/{1}.zip".format(fsrv_port, job_id).encode()]
)

result = broker.recv()
print(result)
