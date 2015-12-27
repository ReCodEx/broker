#!/usr/bin/env python3

# A minimal HTTP server that temporarily saves submitted files
# in a temporary folder and serves them back to workers
# usage:
#       $ python file_server.py

import http.server as http
import socketserver
import cgi
import os
import tempfile
import tarfile

join = os.path.join

port = 9999
tmp = tempfile.TemporaryDirectory()

archive_dir = join(tmp.name, "submit_archives")
os.makedirs(archive_dir)

submit_dir = join(tmp.name, "submits")
os.makedirs(submit_dir)

result_dir = join(tmp.name, "results")
os.makedirs(result_dir)

job_id = 0

class FileServerHandler(http.SimpleHTTPRequestHandler):
    def do_GET(self):
        self.path = join(path, self.path)
        super().do_GET()

    def do_POST(self):
        form = cgi.FieldStorage(
            fp = self.rfile,
            headers = self.headers,
            environ = {
                'REQUEST_METHOD': 'POST',
                'CONTENT_TYPE': self.headers['Content-Type']
            }
        )

        global job_id
        job_id += 1
        job_dir = join(submit_dir, str(job_id))
        os.makedirs(job_dir)

        # Save received files
        for name in form.keys():
            with open(join(job_dir, name), "w") as f:
                f.write(form[name].value)

        # Make an archive from the submitted files
        archive_path = join(archive_dir, str(job_id) + ".tar.gz")
        with tarfile.open(archive_path, "w:gz") as archive:
            archive.add(job_dir, arcname = str(job_id))

        # Response headers
        self.send_response(200)
        self.end_headers()

        # Return the job id assigned to submitted files
        self.wfile.write(str(job_id).encode())

server = socketserver.TCPServer(("", port), FileServerHandler)

print("Serving files from {0} at port {1}...".format(tmp.name, port))
server.serve_forever()
