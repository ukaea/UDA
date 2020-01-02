class UdaException(Exception):
    pass


class ProtocolException(UdaException):
    pass


class ServerException(UdaException):
    pass


class InvalidUseException(UdaException):
    pass
