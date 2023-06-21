import signal
import sys
import time
from concurrent import futures

import app_pb2 as app_messages
import app_pb2_grpc as app_service
import grpc

_ONE_DAY_IN_SECONDS = 60 * 60 * 24


class AppService(app_service.AppServicer):
    def CheckTraceHeader(self, request, context):
        metadata = dict(context.invocation_metadata())
        if "x-ot-span-context" not in metadata:
            context.set_code(grpc.StatusCode.INTERNAL)
            context.set_details("Metadatum x-ot-span-context not found")
        return app_messages.Empty()


def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    app_service.add_AppServicer_to_server(AppService(), server)
    server.add_insecure_port("0.0.0.0:50051")
    server.start()
    signal.signal(signal.SIGTERM, signal.getsignal(signal.SIGINT))
    try:
        while True:
            time.sleep(_ONE_DAY_IN_SECONDS)
    except KeyboardInterrupt:
        server.stop(0)


if __name__ == "__main__":
    serve()
