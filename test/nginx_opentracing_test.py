import http.client
import json
import os
import shutil
import stat
import subprocess
import tempfile
import time
import unittest
import warnings

import app_pb2 as app_messages
import app_pb2_grpc as app_service
import docker
import grpc


def get_docker_client():
    with warnings.catch_warnings():
        # Silence warnings due to use of deprecated methods within dockerpy
        # See https://github.com/docker/docker-py/pull/2931
        warnings.filterwarnings(
            "ignore",
            message="distutils Version classes are deprecated.*",
            category=DeprecationWarning,
        )

        return docker.from_env()


class NginxOpenTracingTest(unittest.TestCase):
    def setUp(self):
        self.testdir = os.getcwd()
        self.workdir = os.path.join(tempfile.mkdtemp(), "environment")
        shutil.copytree(os.path.join(os.getcwd(), "environment"), self.workdir)
        os.chdir(self.workdir)

        # Make sure trace output is writable
        os.chmod(
            os.path.join(self.workdir, "traces", "nginx.json"),
            stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO,
        )

        self.environment_handle = subprocess.Popen(
            ["docker", "compose", "up", "--wait", "--wait-timeout", "60"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        self.client = get_docker_client()
        self.wait_for_containers()

        # Wait so that backend can come up.
        # TODO: replace with something better
        time.sleep(5)

        self.conn = http.client.HTTPConnection("localhost", 8080, timeout=5)

        # Only create the gRPC connection when needed, not during setup
        self.grpcConn = None
        self.running = True

    def wait_for_containers(self, expected_containers=None):
        if expected_containers is None:
            expected_containers = ["nginx", "backend", "php_fpm", "grpc_backend"]

        timeout = time.time() + 120
        while True:
            try:
                containers = {
                    c.name: c.status for c in self.client.containers.list(all=True)
                }

                ready = all(
                    containers.get(expected) == "running"
                    for expected in expected_containers
                )

                if ready:
                    break

                if time.time() > timeout:
                    raise TimeoutError(
                        f"Containers not ready in time. Current containers: {containers}"
                    )
                time.sleep(0.5)
            except docker.errors.APIError as e:
                if time.time() > timeout:
                    raise TimeoutError(f"Docker API error: {str(e)}") from e
                time.sleep(0.5)

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

        shutil.copyfile(
            os.path.join(self.workdir, "traces", "nginx.json"),
            os.path.join(test_log, "nginx-trace.json"),
        )

        shutil.copyfile(
            os.path.join(self.workdir, "logs", "debug.log"),
            os.path.join(test_log, "nginx-debug.log"),
        )
        shutil.copyfile(
            os.path.join(self.workdir, "logs", "error.log"),
            os.path.join(test_log, "nginx-error.log"),
        )

    def _setup_grpc(self):
        """Set up gRPC connection only when needed"""
        if self.grpcConn is None:
            self.grpcConn = grpc.insecure_channel("localhost:8081")
            try:
                grpc.channel_ready_future(self.grpcConn).result(timeout=10)
            except grpc.FutureTimeoutError as err:
                raise RuntimeError("Error connecting to gRPC server") from err
        return self.grpcConn

    def tearDown(self):
        self._stopDocker()
        logdir = None

        if "LOG_DIR" in os.environ:
            self._logEnvironment()

        os.chdir(self.testdir)
        self.client.close()
        self.conn.close()

        # Only close gRPC if it was created
        if self.grpcConn:
            self.grpcConn.close()

    def _stopDocker(self):
        # Close any active gRPC connection before Docker operations that might fork
        if self.grpcConn is not None:
            self.grpcConn.close()
            self.grpcConn = None

        if not self.running:
            return
        self.running = False
        subprocess.check_call(["docker", "compose", "down"])
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

    def testSubrequestTracing(self):
        self.conn.request("GET", "/subrequest")
        response = self.conn.getresponse()
        self.assertEqual(response.status, 200)
        self._stopEnvironment()

        self.assertEqual(len(self.nginx_traces), 4)
        subrequest_span = self.nginx_traces[1]
        request_span = self.nginx_traces[3]
        self.assertEqual(subrequest_span["operation_name"], "/auth")
        self.assertEqual(subrequest_span["tags"]["http.status_code"], 200)
        self.assertEqual(request_span["operation_name"], "/subrequest")
        self.assertEqual(request_span["tags"]["http.status_code"], 200)

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
        self.assertEqual(location_span["tags"]["custom_tag_1"], "123")

    def testMultipleTags(self):
        self.conn.request("GET", "/test-multiple-custom-tags")
        response = self.conn.getresponse()
        self.assertEqual(response.status, 200)
        self._stopEnvironment()

        self.assertEqual(len(self.nginx_traces), 2)

        location_span = self.nginx_traces[0]
        # assert tags that should NOT be in this trace
        self.assertEqual(list(location_span["tags"].values()).count("custom_tag_1"), 0)
        self.assertEqual(
            list(location_span["tags"].values()).count("second_server_tag"), 0
        )
        self.assertEqual(
            list(location_span["tags"].values()).count("second_server_location_tag"), 0
        )
        # assert tags that should be in this trace
        self.assertEqual(location_span["tags"]["first_server_tag"], "dummy1")
        self.assertEqual(location_span["tags"]["custom_tag_2"], "quoted_string")
        self.assertEqual(location_span["tags"]["custom_tag_3"], "another_quoted_string")
        self.assertEqual(location_span["tags"]["custom_tag_4"], "number_123")

    def testFastcgiPropagation(self):
        self.conn.request("GET", "/php-fpm")
        response = self.conn.getresponse()
        self.assertEqual(response.status, 200)
        self._stopEnvironment()

        self.assertEqual(len(self.nginx_traces), 2)

    def testGrpcPropagation(self):
        # Set up gRPC just for this test
        grpc_conn = self._setup_grpc()
        app = app_service.AppStub(grpc_conn)
        try:
            app.CheckTraceHeader(app_messages.Empty())
        finally:
            if self.grpcConn is not None:
                self.grpcConn.close()
                self.grpcConn = None
        self._stopEnvironment()
        self.assertEqual(len(self.nginx_traces), 2)


if __name__ == "__main__":
    unittest.main(verbosity=2)
