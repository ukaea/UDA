class TestClient:
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
