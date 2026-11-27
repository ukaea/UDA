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

        def clear_logs():
            for path in root.glob("*.log"):
                path.unlink()

        def logs():
            return "\n".join(path.read_text(errors="replace") for path in root.glob("*.log"))

        def startup_failure(setting, value, diagnostic):
            clear_logs()
            bad = dict(env)
            bad[setting] = value
            result = subprocess.run([server], env=bad, stdin=subprocess.DEVNULL,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=10)
            assert result.returncode != 0, "invalid policy was accepted"
            assert diagnostic in logs(), (diagnostic, logs(), result.stdout)
            # Also reported on stderr, since the default log level opens no log files.
            assert diagnostic.encode() in result.stdout, result.stdout

        def serve(settings):
            """Run one client request against a server started with env plus settings."""
            clear_logs()
            run_env = dict(env, **settings)
            with socket.socket() as listener:
                listener.bind(("127.0.0.1", 0))
                listener.listen(1)
                listener.settimeout(10)
                request = subprocess.Popen([sys.executable, __file__, "--client", library,
                                            str(listener.getsockname()[1])], env=run_env,
                                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                process = None
                try:
                    with listener.accept()[0] as connection:
                        process = subprocess.Popen([server], env=run_env, stdin=connection,
                                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                    output, _ = request.communicate(timeout=15)
                    process.wait(timeout=5)
                    return request.returncode, output.decode(errors="replace"), process.returncode
                finally:
                    for child in (request, process):
                        if child is not None and child.poll() is None:
                            child.kill()
                            child.wait()

        startup_failure("UDA_ENABLE_PATHS", "/", "has been replaced")
        startup_failure("UDA_LANDLOCK_READONLY_PATHS", "relative", "expected nonempty absolute paths")
        startup_failure("UDA_LANDLOCK", "maybe", "expected ON or OFF")
        if disabled:
            startup_failure("UDA_LANDLOCK_READONLY_PATHS", str(configuration), "built without ENABLE_LANDLOCK")
            startup_failure("UDA_LANDLOCK", "ON", "built without ENABLE_LANDLOCK")
            return
        startup_failure("UDA_LANDLOCK_READONLY_PATHS", str(root / "missing"), "open Landlock path")

        # Fail closed: with no grants configured, the server cannot load its plugin configuration.
        client_status, output, server_status = serve({})
        assert client_status != 0, "request succeeded with no Landlock grants:\n" + output
        # Shutdown must be clean even though no plugin ran (no redirect file to close).
        assert server_status == 0, f"server exited {server_status} after a denied request"
        assert "Landlock filesystem policy enforced" in logs(), logs()

        # An explicit OFF disables enforcement even when grants are configured.
        client_status, output, server_status = serve({"UDA_LANDLOCK": "OFF",
                                                      "UDA_LANDLOCK_READONLY_PATHS": str(configuration)})
        assert client_status == 0, output + "\n" + logs()
        assert server_status == 0, f"server exited {server_status}"
        assert "Landlock disabled by UDA_LANDLOCK=OFF" in logs(), logs()
        assert "Landlock filesystem policy enforced" not in logs(), logs()

        # Libraries are readable, scratch is writable, and no paths grant execution.
        # Logs were opened before enforcement.
        # Include all UDA dependency libraries in the build tree, not just libuda_client.
        readable = [str(configuration), str(build_root)]
        readable += [path for path in ("/usr", "/lib", "/lib64", "/etc/ld.so.cache") if Path(path).exists()]
        client_status, output, server_status = serve({"UDA_LANDLOCK_READONLY_PATHS": ":".join(readable),
                                                      "UDA_LANDLOCK_READWRITE_PATHS": str(scratch)})
        assert client_status == 0, output + "\n" + logs()
        assert server_status == 0, f"server exited {server_status}"
        assert "Landlock filesystem policy enforced" in logs(), logs()

if __name__ == "__main__":
    if sys.argv[1] == "--client":
        client(*sys.argv[2:])
    else:
        integration(*sys.argv[1:4], disabled="--disabled" in sys.argv[4:])
