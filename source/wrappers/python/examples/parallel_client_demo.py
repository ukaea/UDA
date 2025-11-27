import pyuda
import multiprocessing as mp
from enum import Enum, auto
from collections import namedtuple

ConnectionDetails = namedtuple('ConnectionDetails', ['name', 'port'])
RequestData = namedtuple('RequestData', ['signal', 'source'])
# client = pyuda.Client()

class Result(Enum):
    SOME = auto()
    ERROR = auto()


def task(queue: mp.Queue, server: ConnectionDetails, request: RequestData):
    client = pyuda.Client()
    pyuda.Client.port = server.port
    pyuda.Client.server = server.name

    try:
        result = client.get(*request)
        queue.put((Result.SOME, result))
    except Exception as e:
        queue.put((Result.ERROR, e))


def main():
    server = ConnectionDetails("uda2.mast.l", 56565)
    request = RequestData("help::help()", "")
    results_q = mp.Queue()
    p = mp.Process(target=task, args=(results_q, server, request))
    p.start()
    item = results_q.get()
    print(item)
    p.join()


if __name__ == "__main__":
    main()
