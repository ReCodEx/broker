#!/usr/bin/env python3

# Send a fake submit to the ReCodEx broker
# usage:
#       $ python fake_submit.py --header1 val1 --header2 val2 submit_directory

import zmq
import yaml
import os
import sys
import requests
from glob import iglob

fsrv_port = 9999
broker_port = 9658

submit_dir = ""
headers = {}
argv_it = iter(sys.argv)

# Load arguments
script_name = next(argv_it)

for arg in argv_it:
    if arg.startswith("--") and len(arg) > 2:
        headers[arg[2:]] = next(argv_it)
    else:
        submit_dir = arg

if submit_dir == "":
    sys.exit("no directory to submit was specified")

# Make an iterator for the submitted files
files = (
    os.path.relpath(f, submit_dir) 
    for f in iglob(submit_dir + "/**", recursive = True)
    if os.path.isfile(f)
)

# Send the submission to our fake file server
reply = requests.post(
    "http://localhost:{0}".format(fsrv_port),
    {f.encode(): open(os.path.join(submit_dir, f), "rb").read() for f in files}
)
job_id = reply.text

# Connect to the broker
context = zmq.Context()
broker = context.socket(zmq.REQ)
broker.connect("tcp://localhost:{}".format(broker_port))

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
