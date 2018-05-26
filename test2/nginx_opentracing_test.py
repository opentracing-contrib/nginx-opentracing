import unittest
import shutil
import os
import tempfile
import subprocess
import time
import docker
import json
import http.client

class NginxOpenTracingTest(unittest.TestCase):
    def setUp(self):
        print("********* setting up environment ****")
        self.testdir = os.getcwd()
        self.workdir = os.path.join(tempfile.mkdtemp(), "environment")
        shutil.copytree(os.path.join(os.getcwd(), "environment"),
                        self.workdir)
        os.chdir(self.workdir)
        self.environment_handle = subprocess.Popen(["docker-compose", "up"], 
                                                   stdout=subprocess.PIPE, 
                                                   stderr=subprocess.PIPE)
        self.client = docker.from_env()
        timeout = time.time() + 5
        print("********* waiting for containers to come up ****")
        while len(self.client.containers.list()) != 2:
            if time.time() > timeout:
                raise TimeoutError()
            time.sleep(0.001)

        # Wait so that backend can come up.
        # TODO: replace with something better
        time.sleep(3)

        print("****** connecting to nginx *****")
        self.conn = http.client.HTTPConnection('localhost', 8080, timeout=5)
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
        print("**** sending request ***")
        self.conn.request("GET", "/")
        print("**** getting response ***")
        response = self.conn.getresponse()
        self.assertEqual(response.status, 200)
        print("**** stopping environment ***")
        self._stopEnvironment()

        self.assertEqual(len(self.nginx_traces), 2)

if __name__ == '__main__':
    unittest.main()
