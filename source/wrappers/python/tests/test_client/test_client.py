class TestClient:
    __test__ = False  # not a pytest test class

    _exported_methods = [
            "speak",
            "greet"
            ]

    @classmethod
    def register(cls, client):
        subclient = cls(client)
        for method in cls._exported_methods:
            client.register_method(method, subclient)

    def __init__(self, client):
        self.client = client

    def speak(self):
        return "woof"

    def greet(self):
        return "hello from the test sub-client"


class NoRegisterClient:
    """Subclient from an old package that predates the register() interface"""

    def __init__(self, client):
        self.client = client
