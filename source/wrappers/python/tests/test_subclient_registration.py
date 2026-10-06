import pytest
import pyuda
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
def test_env_parser(monkeypatch, input_string, expected):
    monkeypatch.setenv("UDA_SUBCLIENTS", input_string)
    assert _parse_subclient_register_from_env() == expected


def test_env_parser_throws_when_var_unset(monkeypatch):
    monkeypatch.delenv("UDA_SUBCLIENTS", raising=False)
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
                             pytest.param("wrong.[Wrong",
                                          UdaSubclientsStringError,
                                          id="non_alphanumeric_character"),
                             ])
def test_env_parser_throws_when_var_misformatted(monkeypatch, input_string, error):
    monkeypatch.setenv("UDA_SUBCLIENTS", input_string)
    with pytest.raises(error):
        _parse_subclient_register_from_env()


def test_empty_string_skips_subclient_registration(monkeypatch, recwarn):
    monkeypatch.setenv("UDA_SUBCLIENTS", "")
    client = pyuda.Client()
    for warning in recwarn:
        assert not issubclass(warning.category, pyuda.UdaSubclientDeprecationWarning)
    assert client._registered_subclients == {}


def test_register_subclient_manually_from_classmethod(monkeypatch):
    monkeypatch.setenv("UDA_SUBCLIENTS", "")
    client = pyuda.Client()
    test_client.TestClient.register(client)
    assert client.speak() == "woof"


def test_register_subclient_manually_from_pyuda_client(monkeypatch):
    monkeypatch.setenv("UDA_SUBCLIENTS", "")
    client = pyuda.Client()
    client.register_subclient(test_client.TestClient)
    assert client.speak() == "woof"


def test_register_sub_client_method_from_env(monkeypatch):
    monkeypatch.setenv("UDA_SUBCLIENTS", "test_client.TestClient")
    client = pyuda.Client()
    assert client.speak() == "woof"


def test_reregistering_method_warns_and_overwrites(monkeypatch):
    monkeypatch.setenv("UDA_SUBCLIENTS", "test_client.TestClient")
    client = pyuda.Client()
    previous = client._registered_subclients["speak"]
    with pytest.warns(UserWarning, match="overwritten by TestClient"):
        client.register_subclient(test_client.TestClient)
    assert client._registered_subclients["speak"] is not previous
    assert client.speak() == "woof"


def test_subclient_without_register_method_falls_back_to_legacy(monkeypatch):
    calls = []
    monkeypatch.setattr(pyuda.Client, "register_legacy_subclients", lambda self: calls.append(self))
    monkeypatch.setenv("UDA_SUBCLIENTS", "test_client.test_client.NoRegisterClient")
    with pytest.warns(pyuda.UdaSubclientDeprecationWarning) as record:
        client = pyuda.Client()
    assert calls == [client]
    assert str(record[0].message).startswith("WARNING: one of the subclient classes specified")


def test_module_not_found_raised(monkeypatch):
    monkeypatch.setenv("UDA_SUBCLIENTS", "not_installed.FakeClient")
    with pytest.raises(ImportError):
        pyuda.Client()


def test_module_not_found_for_subclient_dependency(monkeypatch):
    monkeypatch.setenv("UDA_SUBCLIENTS", "breaking_client.FakeClient")
    with pytest.raises(ImportError):
        pyuda.Client()
