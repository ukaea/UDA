import pytest
import pyuda
import os
# import warnings
from pyuda._client import _parse_subclient_register_from_env, UdaSubclientsStringError
import test_client


@pytest.mark.parametrize("input_string,expected",
                         [
                             pytest.param("", {}, id="empty_string"),
                             pytest.param("test.TestClient",
                                          {"test": ["TestClient"]},
                                          id="single_correct_entry"
                                          ),
                             pytest.param("mast.MastClient:mast.geom.GeomClient",
                                          {"mast": ["MastClient"],
                                           "mast.geom": ["GeomClient"]},
                                          id="multiple_correct_entries"
                                          ),
                             pytest.param("mast.MastClient:mast.MastClient2:geom.GeomClient:geom.GeomClient2",
                                          {"mast": ["MastClient", "MastClient2"],
                                           "geom": ["GeomClient", "GeomClient2"]},
                                          id="multiple_nested_entries"
                                          ),
                         ])
def test_env_parser(input_string, expected):
    os.environ["UDA_SUBCLIENTS"] = input_string
    assert _parse_subclient_register_from_env() == expected


def test_env_parser_throws_when_var_unset():
    del os.environ["UDA_SUBCLIENTS"]
    with pytest.raises(KeyError):
        _parse_subclient_register_from_env()


@pytest.mark.parametrize("input_string,error",
                         [
                             pytest.param("wrong",
                                          UdaSubclientsStringError,
                                          id="no_module_separators"),
                             pytest.param("wrong.Wrong;not_right.NotRight",
                                          UdaSubclientsStringError,
                                          id="wrong_delimiter"),
                             ])
def test_env_parser_throws_when_var_misformatted(input_string, error):
    os.environ["UDA_SUBCLIENTS"] = input_string
    with pytest.raises(error):
        _parse_subclient_register_from_env()


def test_empty_string_skips_subclient_registration(recwarn):
    os.environ["UDA_SUBCLIENTS"] = ""
    client = pyuda.Client()
    for warning in recwarn:
        assert not issubclass(warning.category, pyuda.SubClientDeprecationWarning)
    assert client._registered_subclients == {}


def test_register_subclient_manually_from_classmethod():
    os.environ["UDA_SUBCLIENTS"] = ""
    client = pyuda.Client()
    test_client.TestClient.register(client)
    assert client.speak() == "woof"


def test_register_subclient_manually_from_pyuda_client():
    os.environ["UDA_SUBCLIENTS"] = ""
    client = pyuda.Client()
    client.register_subclient(test_client.TestClient)
    assert client.speak() == "woof"


def test_register_sub_client_method_from_env():
    os.environ["UDA_SUBCLIENTS"] = "test_client.TestClient"
    client = pyuda.Client()
    assert client.speak() == "woof"


def test_module_not_found_raised():
    os.environ["UDA_SUBCLIENTS"] = "not_installed.FakeClient"
    with pytest.raises(ModuleNotFoundError):
        pyuda.Client()


def test_module_not_found_for_subclient_dependency():
    os.environ["UDA_SUBCLIENTS"] = "breaking_client.FakeClient"
    with pytest.raises(ModuleNotFoundError):
        pyuda.Client()
