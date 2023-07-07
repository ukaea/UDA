import pyuda
# import mast
# from mast.geom.geometryClient import GeomClient
# from mast import MastClient


def void_display(*args, **kwargs):
    pass


# monkey patch any methods which matlab cannot parse, particularly for reteiving structured data here
pyuda.StructuredData._display = void_display


# inherit from all clients and subclients to automatically populate all available wrapper functionswithout writing them all again here.
# Matlab doesn't automatically pick up the registered subclients like in pyuda
class Client(pyuda.Client):
    def __init__(self):
        pyuda.Client.__init__(self)
        # MastClient.__init__(self, pyuda.Client())
        # GeomClient.__init__(self, pyuda.Client())

    def set_port(self, number):
        pyuda.Client.port = number

    def get_port(self):
        return pyuda.Client.port

    def set_server(self, address):
        pyuda.Client.server = address

    def get_server(self):
        return pyuda.Client.server
