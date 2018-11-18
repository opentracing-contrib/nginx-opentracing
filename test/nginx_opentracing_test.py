import unittest
import shutil
import os
import stat
import tempfile
import subprocess
import time
import docker
import json
import http.client
import grpc
import app_pb2 as app_messages
import app_pb2_grpc as app_service

class NginxOpenTracingTest(unittest.TestCase):
    def setUp(self):
        self.testdir = os.getcwd()
        self.workdir = os.path.join(tempfile.mkdtemp(), "environment")
        shutil.copytree(os.path.join(os.getcwd(), "environment"),
                        self.workdir)
        os.chdir(self.workdir)

        # Make sure trace output is writable
        os.chmod(os.path.join(self.workdir, "traces", "nginx.json"), 
                stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)

        self.environment_handle = subprocess.Popen(["docker-compose", "up"], 
                                                   stdout=subprocess.PIPE, 
                                                   stderr=subprocess.PIPE)
        self.client = docker.from_env()
        timeout = time.time() + 60
        while len(self.client.containers.list()) != 4:
            if time.time() > timeout:
                raise TimeoutError()
            time.sleep(0.001)

        # Wait so that backend can come up.
        # TODO: replace with something better
        time.sleep(2)

        self.conn = http.client.HTTPConnection('localhost', 8080, timeout=5)
        self.grpcConn = grpc.insecure_channel('localhost:8081')
        try:
            grpc.channel_ready_future(self.grpcConn).result(timeout=10)
        except grpc.FutureTimeoutError:
            sys.exit('Error connecting to server')
        self.running = True

    def _logEnvironment(self):
        logdir = os.environ["LOG_DIR"]
        test_log = os.path.join(logdir, self._testMethodName)
        if os.path.exists(test_log):
            shutil.rmtree(test_log)
        os.mkdir(test_log)

        with open(os.path.join(test_log, "docker_compose_stdout"), "w") as out:
            out.write(str(self.environment_stdout))
        with open(os.path.join(test_log, "docker_compose_stderr"), "w") as out:
            out.write(str(self.environment_stderr))

        shutil.copyfile(os.path.join(self.workdir, "traces", "nginx.json"),
                        os.path.join(test_log, "nginx-trace.json"))

        shutil.copyfile(os.path.join(self.workdir, "logs", "debug.log"),
                        os.path.join(test_log, "nginx-debug.log"))
        shutil.copyfile(os.path.join(self.workdir, "logs", "error.log"),
                        os.path.join(test_log, "nginx-error.log"))

    def tearDown(self):
        self._stopDocker()
        logdir = None

        if "LOG_DIR" in os.environ:
            self._logEnvironment()

        os.chdir(self.testdir)
        self.client.close()
        self.conn.close()
        self.grpcConn.close()

    def _stopDocker(self):
        if not self.running:
            return
        self.running = False
        subprocess.check_call(["docker-compose", "down"])
        stdout, stderr = self.environment_handle.communicate()
        self.environment_stdout = stdout
        self.environment_stderr = stderr
        self.environment_returncode = self.environment_handle.returncode

    def _stopEnvironment(self):
        if not self.running:
            return
        self._stopDocker()
        self.assertEqual(self.environment_returncode, 0)
        with open(os.path.join(self.workdir, "traces", "nginx.json")) as f:
            self.nginx_traces = json.load(f)

    def testTrivialRequest(self):
        self.conn.request("GET", "/")
        response = self.conn.getresponse()
        self.assertEqual(response.status, 200)
        self._stopEnvironment()

        self.assertEqual(len(self.nginx_traces), 2)

    def testManualPropagation(self):
        self.conn.request("GET", "/manual-propagation")
        response = self.conn.getresponse()
        self.assertEqual(response.status, 200)
        self._stopEnvironment()

        self.assertEqual(len(self.nginx_traces), 2)

    def testNoTraceLocations(self):
        self.conn.request("GET", "/no-trace-locations")
        response = self.conn.getresponse()
        self.assertEqual(response.status, 200)
        self._stopEnvironment()

        self.assertEqual(len(self.nginx_traces), 1)

    def testInternalRediect(self):
        self.conn.request("GET", "/internal-redirect")
        response = self.conn.getresponse()
        self.assertEqual(response.status, 200)
        self._stopEnvironment()

        self.assertEqual(len(self.nginx_traces), 3)

    def testCustomTag(self):
        self.conn.request("GET", "/custom-tag")
        response = self.conn.getresponse()
        self.assertEqual(response.status, 200)
        self._stopEnvironment()

        self.assertEqual(len(self.nginx_traces), 2)

        location_span = self.nginx_traces[0]
        self.assertEqual(location_span["tags"]["abc"], "123")

    def testFastcgiPropagation(self):
        self.conn.request("GET", "/php-fpm")
        response = self.conn.getresponse()
        self.assertEqual(response.status, 200)
        self._stopEnvironment()

        self.assertEqual(len(self.nginx_traces), 2)

    def testGrpcPropagation(self):
        app = app_service.AppStub(self.grpcConn)
        app.CheckTraceHeader(app_messages.Empty())

        self._stopEnvironment()

        self.assertEqual(len(self.nginx_traces), 2)

if __name__ == '__main__':
    unittest.main(verbosity=2)
