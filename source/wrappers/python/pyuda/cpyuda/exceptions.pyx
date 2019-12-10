class UDAException(Exception):
    pass


class ProtocolException(UDAException):
    pass


class ServerException(UDAException):
    pass


class ClientException(UDAException):
    pass


class InvalidUseException(UDAException):
    pass
