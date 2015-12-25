#!/usr/bin/env/python3

# Send a fake submit to the ReCodEx broker
# usage:
#       $ python fake_submit.py --header1 val1 --header2 val2
# The script assumes that there is a config.yml file in its 
# working directory. If you don't want to supply your own file,
# run the script in the examples directory.

import zmq
import yaml
import sys

headers = {}
argv_it = iter(sys.argv)

for arg in argv_it:
    if arg.startswith("--"):
        headers[arg[2:]] = next(argv_it)

with open("config.yml") as config_file:
    config = yaml.load(config_file)

context = zmq.Context()
broker = context.socket(zmq.REQ)
broker.connect("tcp://localhost:{}".format(config['client_port']))

broker.send_multipart(
    [b"eval"] +
    ["{}={}".format(k, v).encode() for k, v in headers.items()] +
    [b""] +
    [b"http://localhost:9999/submission_archives/example.tar.gz"] +
    [b"http://localhost:9999/results/example/"]
)

result = broker.recv()
print(result)
