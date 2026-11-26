"""Exercise startup validation and a real plugin request under Landlock."""
import ctypes
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile


def client(library, port):
    uda = ctypes.CDLL(library, mode=ctypes.RTLD_GLOBAL)
    uda.idamGetAPIWithHost.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
    uda.getIdamErrorCode.argtypes = [ctypes.c_int]
    uda.getIdamErrorMsg.argtypes = [ctypes.c_int]
    uda.getIdamErrorMsg.restype = ctypes.c_char_p
    handle = uda.idamGetAPIWithHost(b"help::help()", b"", b"127.0.0.1", int(port))
    if handle < 0 or uda.getIdamErrorCode(handle) != 0:
        raise RuntimeError(f"plugin request failed: handle={handle}, error={uda.getIdamErrorMsg(handle)!r}")


def integration(server, library, plugin, disabled=False):
    with tempfile.TemporaryDirectory(prefix="uda-landlock-server-") as temporary:
        root = Path(temporary)
        scratch = root / "scratch"
        scratch.mkdir()
        configuration = root / "plugins.conf"
        configuration.write_text(f"HELP,function,helpPlugin,{Path(plugin).name},*,1,0,1,Help,help::help()\n")
        env = {key: value for key, value in os.environ.items()
               if not key.startswith("UDA_") and key != "LD_PRELOAD"}
        env.update(UDA_LOG=temporary, UDA_LOG_LEVEL="DEBUG", UDA_LOG_MODE="a",
                   UDA_PLUGIN_CONFIG=str(configuration), UDA_PLUGIN_DIR=str(Path(plugin).parent),
                   UDA_PLUGIN_FAIL_ON_LOAD="1", UDA_SERVER_SSL_AUTHENTICATE="OFF",
                   UDA_CLIENT_SSL_AUTHENTICATE="OFF", UDA_ALLOWED_PATHS="/")
        env["UDA_PLUGIN_REDIRECT_DIR"] = str(scratch)
        build_root = Path(server).resolve().parents[2]
        # Plugins use install RPATHs, so resolve dependencies from this uninstalled build explicitly.
        library_paths = sorted({str(path.parent) for path in build_root.rglob("*.so")})
        library_paths += [path for path in env.get("LD_LIBRARY_PATH", "").split(":") if path]
        env["LD_LIBRARY_PATH"] = ":".join(library_paths)

        def startup_failure(setting, value, diagnostic):
            bad = dict(env)
            bad[setting] = value
            result = subprocess.run([server], env=bad, stdin=subprocess.DEVNULL,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=10)
            logs = "\n".join(path.read_text(errors="replace") for path in root.glob("*.log"))
            assert result.returncode != 0, "invalid policy was accepted"
            assert diagnostic in logs, (diagnostic, logs, result.stdout)

        startup_failure("UDA_ENABLE_PATHS", "/", "has been replaced")
        startup_failure("UDA_LANDLOCK_READONLY_PATHS", "relative", "expected nonempty absolute paths")
        if disabled:
            startup_failure("UDA_LANDLOCK_READONLY_PATHS", str(configuration), "built without ENABLE_LANDLOCK")
            return
        startup_failure("UDA_LANDLOCK_READONLY_PATHS", str(root / "missing"), "open Landlock path")

        # Libraries are readable, scratch is writable, and no paths grant execution.
        # Logs were opened before enforcement.
        # Include all UDA dependency libraries in the build tree, not just libuda_client.
        readable = [str(configuration), str(build_root)]
        readable += [path for path in ("/usr", "/lib", "/lib64", "/etc/ld.so.cache") if Path(path).exists()]
        env["UDA_LANDLOCK_READONLY_PATHS"] = ":".join(readable)
        env["UDA_LANDLOCK_READWRITE_PATHS"] = str(scratch)
        with socket.socket() as listener:
            listener.bind(("127.0.0.1", 0))
            listener.listen(1)
            listener.settimeout(10)
            request = subprocess.Popen([sys.executable, __file__, "--client", library,
                                        str(listener.getsockname()[1])], env=env,
                                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            process = None
            try:
                with listener.accept()[0] as connection:
                    process = subprocess.Popen([server], env=env, stdin=connection,
                                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                output, _ = request.communicate(timeout=15)
                errors = (root / "Error.log").read_text(errors="replace")
                assert request.returncode == 0, output.decode(errors="replace") + "\n" + errors
                process.wait(timeout=5)
                assert process.returncode == 0, f"server exited {process.returncode}"
                assert "Landlock filesystem policy enforced" in (root / "DebugServer.log").read_text()
            finally:
                for child in (request, process):
                    if child is not None and child.poll() is None:
                        child.kill()
                        child.wait()


if __name__ == "__main__":
    if sys.argv[1] == "--client":
        client(*sys.argv[2:])
    else:
        integration(*sys.argv[1:4], disabled="--disabled" in sys.argv[4:])
