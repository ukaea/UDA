---
layout: default
title: Server sandboxing
parent: Server Installation
nav_order: 1
---

# Server sandboxing
{:.no_toc}

UDA's optional Landlock policy limits filesystem access inside the Linux server.
System administrators can combine it with service-manager restrictions, mandatory
access control, or container isolation. This page explains which boundary each
control establishes and how to configure UDA's guardrail.

## Contents
{:.no_toc}

1. TOC
{:toc}

## In-process guardrail and deployment policy

**In-process** describes where UDA activates Landlock; the Linux kernel enforces the
installed rules. UDA reads the configured paths during startup, creates the ruleset,
and restricts itself before SSL and plugin initialization. A subsequent plugin bug
cannot simply turn that policy off. However, the server executable, its initial
library loading, log creation and other work before activation are outside this
particular guardrail. The shell startup script runs before it as well.

An **external runtime policy** is established by the service manager, a sandbox
launcher or the host's security policy. It can constrain the launcher and server
before UDA reaches its own initialization code, and can operate without a
Landlock-enabled UDA build. Its coverage depends on the mechanism and policy.

Landlock is a general Linux facility and can also be used by external launchers;
UDA's implementation embeds the setup in the server so its permission classes are
part of UDA configuration. Changing those environment variables requires launching
new server processes. They do not modify an already installed ruleset.

| Control | Who configures it | What it covers |
| --- | --- | --- |
| `UDA_ALLOWED_PATHS` | UDA administrator | Application-level path checks in plugins that use the check; not a kernel boundary for arbitrary plugin I/O |
| UDA Landlock variables | UDA administrator; enforced by the kernel after UDA applies them | Selected filesystem operations by the calling thread and future children |
| systemd sandbox settings | Service administrator | Service launch environment, filesystem namespaces and optional resource/syscall restrictions |
| AppArmor or SELinux policy | Host administrator | Kernel-enforced mandatory access control for the deployed process/domain |
| Bubblewrap or container runtime | Launcher/deployment administrator | Filesystem and other isolation selected by the runtime configuration |
| macOS Seatbelt profile | Native macOS launcher/application | macOS process restrictions; a separate implementation from Landlock |

Use a dedicated service account and ordinary file ownership/permissions in every
case. A sandbox grant does not override those permissions. When multiple controls
apply, an operation must satisfy all of them: adding a Landlock writable path does
not make a read-only container mount writable or bypass an AppArmor denial.
Landlock is designed to coexist with other Linux security policies; see the
[kernel overview](https://docs.kernel.org/userspace-api/landlock.html).

Protect the executable, plugin libraries, startup scripts and configuration from
modification by untrusted users. The environment is trusted deployment input: an
administrator can omit UDA's policy variables before launch, and a modified server
could omit its own guardrail. An independently managed external policy can retain
a boundary in those cases.

## Configuring UDA's Landlock policy

Build with `-DENABLE_LANDLOCK=ON`. Configuration requires Linux headers providing
Landlock ABI 3 definitions, and enforcement requires an enabled kernel
implementation with ABI 3 or newer. UDA fails startup if a requested policy cannot
be enforced, rather than dropping restrictions unsupported by the kernel. The build
option defaults to `OFF`; explicitly enabling it for a non-Linux target is an error.

Set any of these environment variables in `udaserver.cfg` to enable enforcement:

| Variable | Directory grant | Individual file grant |
| --- | --- | --- |
| `UDA_LANDLOCK_READONLY_PATHS` | Read files and list directories beneath the path | Read the file |
| `UDA_LANDLOCK_READWRITE_PATHS` | Read, write, truncate, create, remove, link and rename beneath the path | Read, write and truncate the existing file |
| `UDA_LANDLOCK_EXECUTE_PATHS` | Read, list and execute beneath the path | Read and execute the file |

**Read and read/write grants do not imply execution.** An executable must be granted
execution explicitly. Execute grants do not grant write permission. Permissions are
additive: a writable subdirectory inside a read-only tree is writable, and a path
listed as both read/write and executable receives both grants. A narrower read-only
entry cannot revoke permission granted by a writable ancestor.

Lists use `:` separators and require absolute paths. Paths must already exist when
the server starts; create scratch directories before launching it. Spaces in paths
are supported when the environment variable is quoted, but `:` in a filename cannot
be represented. Relative paths and empty components such as `/data:` or `/data::/tmp`
are rejected. Symlinks in configured paths resolve to their targets; access checks
follow filesystem objects, rather than trusting a textual path prefix. Directory
grants include descendants. A file grant does not permit access to sibling files or
allow creation, deletion or renaming through its parent directory.

If **all three variables are unset**, Landlock is disabled. Setting a variable to an
empty string enables the policy but adds no grants for that class. Setting all
configured classes empty denies new opens for the handled operations everywhere;
it will normally prevent successful server startup. A configured policy also causes
startup to fail in a build without Landlock support.

When the policy cannot be parsed or applied, the server exits before it can send
an error to the client, so the client reports only a closed connection or protocol
error. The cause is written to the server's stderr, which the supplied
`udaserver.sh` sends to `etc/server.log`, and to `Error.log` when `UDA_LOG_LEVEL`
enables logging. The legacy `UDA_ENABLE_PATHS`
variable is rejected with migration instructions; replace it with
the appropriate permission classes. `UDA_ALLOWED_PATHS` remains a separate
application-level plugin check, with its existing semicolon-separated syntax.

For example, after creating `/var/tmp/uda`, configure a read-only data tree, runtime
files, writable scratch space and the programs the server runs itself (shown for
x86-64):

```sh
export UDA_LANDLOCK_READONLY_PATHS="$UDA_ROOT/etc:$UDA_ROOT/lib:/srv/uda/data:/usr/lib:/usr/lib64:/lib:/lib64:/etc/ld.so.cache"
export UDA_LANDLOCK_READWRITE_PATHS="/var/tmp/uda"
export UDA_PLUGIN_REDIRECT_DIR="/var/tmp/uda"
export UDA_WORK_DIR="/var/tmp/uda"
export UDA_LANDLOCK_EXECUTE_PATHS="/bin/sh:/usr/bin/head:/lib64/ld-linux-x86-64.so.2"
```

Every listed path must exist, so remove entries your distribution does not have
(for example `/usr/lib64` or `/lib64` on some Debian-based systems). The library
directories must cover wherever plugin dependencies are installed, including
multiarch directories such as `/usr/lib/x86_64-linux-gnu` and `lib64` directories on
RHEL-like systems. Without read access to `/etc/ld.so.cache`, the dynamic loader
only searches its default trusted directories and `LD_LIBRARY_PATH`.

**The server runs external programs even without plugins that do.** To detect the
format of a requested file that has no extension, it runs `head -c10 <file>`
through `/bin/sh` with `popen()`. For HDF-labelled files it also runs the
programs named by `UDA_DUMP_NETCDF` and `UDA_DUMP_IDA` when those are set. Grant
execute permission to `/bin/sh`, `head`, any configured dump programs and the ELF
interpreter they use. Without these grants, requests for files without extensions
fail with "failed to read command".

`UDA_PLUGIN_REDIRECT_DIR` directs plugin output capture to the permitted scratch
directory; without it, that capture uses `/tmp`, which would need its own writable
grant. `UDA_WORK_DIR` likewise holds the temporary XDR files used for structured
data in server-to-server requests; it also defaults to `/tmp`. Adapt the runtime
paths to your deployment; include plugin configuration, plugin libraries and their
dependencies, certificates, and any other required reads.
Libraries loaded with `dlopen()` need read access, not Landlock execute permission.
When launching an external program, grant its path execute permission **and** grant
the ELF interpreter (dynamic loader) or script interpreter execute permission. Its
libraries and other inputs also need read access. For example, an AArch64 deployment
may need `/lib/ld-linux-aarch64.so.1`; determine the loader for the actual executable
rather than copying a path for a different architecture.

## Scope and limitations

The current policy applies to `uda_server`. The experimental `uda_server2` and
embedded fat client do not activate it. Installing Landlock on the host alone does
not sandbox every UDA process; the configured, enabled server must apply its policy.

UDA activates the policy during single-threaded startup, after opening its logs and
before initializing SSL and plugins. Restrictions cannot be removed by the confined
thread and are inherited by future children. Existing sibling threads are not
synchronized by this implementation. Already-open descriptors retain their access,
including the log streams; do not treat activation as revoking previously opened
files or established connections.

This implementation handles filesystem read, write, creation, removal, linking,
renaming, truncation and execution rights. It does not configure network restrictions,
resource limits, or all filesystem metadata operations, such as permission changes.
Newer Landlock ABIs have additional capabilities; UDA does not automatically enable
all of them. Creating character and block devices is denied even in writable roots.

Execute permission governs kernel execution of files. It does not prevent an allowed
interpreter from evaluating a readable script or prevent executable mappings of
readable libraries. Treat loaded native plugins as code running with the server's
remaining authority, not as mutually isolated components. See the
[kernel filesystem rights and limitations](https://docs.kernel.org/userspace-api/landlock.html#filesystem-flags).

Client authentication and authorization are separate concerns: a filesystem sandbox
limits what the process can access, not which client may request a particular dataset.
See [authentication](/UDA/authentication).

## Alternatives and complementary controls

There is no single Linux command that is a direct equivalent of macOS Seatbelt.
Choose according to whether the requirement is a service filesystem policy,
centrally enforced access control, or a more isolated execution environment.
The following are administrator choices, not extra backends selected by UDA's
Landlock variables.

### systemd service sandboxing

For a systemd deployment, settings in the service unit or a drop-in can constrain
the service without changing UDA. `ProtectSystem=strict` makes most of the filesystem
read-only; use `ReadWritePaths=` or managed state/log directories for writable areas.
**Read-only does not mean unreadable**: use `InaccessiblePaths=` or a deliberately
restricted filesystem view when confidential paths must be hidden.

`ProtectHome=`, `PrivateTmp=` and `NoNewPrivileges=` address additional service
requirements. Private temporary directories change what `/tmp` means to the
service; align UDA's scratch paths with that view. Availability and behavior depend
on the installed systemd, kernel and whether this is a system or user service.
Consult the [systemd execution-environment reference](https://www.freedesktop.org/software/systemd/man/latest/systemd.exec.html).

Apply settings to the service template that launches UDA's socket-activated workers,
not just the listening socket unit. Preserve `StandardInput=socket`. Account for the
startup script's dependencies and writes before locking down its filesystem. In
particular, UDA's supplied script writes startup/server logs under the install
prefix's `etc` directory; relocate those outputs or explicitly permit their directory.
A systemd policy can protect this earlier startup phase while Landlock narrows the
server's subsequent filesystem access.

### AppArmor and SELinux

[AppArmor](https://apparmor.net/) uses process profiles, including pathname-oriented
file rules. It suits sites that already manage application profiles and want an
administrator-enforced boundary covering UDA and its executable transitions.
Ensure the relevant profile is loaded in enforcing mode; merely having AppArmor
installed is insufficient.

[SELinux](https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/9/html/using_selinux/getting-started-with-selinux_using-selinux)
uses labels, process domains and policy rules. It suits deployments with established
SELinux policy management, particularly when access should follow labeled resources
rather than only installation paths. Check file labels, domain transitions and
actual enforcing status. Both approaches require a policy adapted to the plugins,
data mounts and network access of the deployment.

### Bubblewrap, containers and virtual machines

[Bubblewrap](https://github.com/containers/bubblewrap) constructs a restricted
execution environment using Linux namespaces and selected mounts. It can wrap a
server without changing its code; the supplied arguments determine the boundary,
and user-namespace availability varies across hosts.

A [container runtime](https://docs.docker.com/engine/security/) can expose only selected
data mounts, make the root filesystem read-only, and provide separate network and
resource configuration. Containers share their Linux host kernel; broad writable
host mounts or privileged execution weaken isolation. A virtual machine adds a
separate guest kernel, at the cost of managing that guest and its data access.

Landlock can provide an additional filesystem restriction inside these environments.
Paths are interpreted as seen by the server, so write the policy using container
paths. The runtime must permit the required syscalls, and the host or guest Linux
kernel must enable the required Landlock ABI. Installing headers in a container
cannot add kernel support.

### Seccomp

[Seccomp filters](https://docs.kernel.org/userspace-api/seccomp_filter.html) restrict
system calls and scalar arguments. They complement filesystem restrictions by
reducing the available kernel interface, but cannot safely dereference a pathname
argument to implement a file allowlist. UDA's production Landlock setup does not
install a seccomp filter. A service or runtime can add one, provided it permits
Landlock setup and the calls needed by UDA and its plugins.

### macOS Seatbelt and App Sandbox

Seatbelt is macOS's kernel sandbox mechanism. A profile can restrict filesystem,
network and other process operations. An external launcher can use:

```sh
sandbox-exec -f uda-development.sb /path/to/uda_server
```

This illustrates where a separately maintained policy would be applied; UDA does
not ship `uda-development.sb` or translate its Landlock variables into a Seatbelt
profile. The profile must allow the loader, libraries, system services, files and
connections required by the chosen launch command. Chromium documents its own
[Seatbelt integration](https://github.com/chromium/chromium/blob/main/sandbox/mac/README.md).

The `sandbox-exec(1)` and `sandbox_init(3)` interfaces are deprecated in Apple's
macOS manuals. Their deprecation is distinct from removal of Seatbelt enforcement:
these interfaces are still present on macOS 26.3.1. Check `man sandbox-exec`,
`man 3 sandbox_init` and availability on the target release; do not assume indefinite compatibility.
Apple directs application developers to [App Sandbox](https://developer.apple.com/documentation/security/app-sandbox),
which uses signing entitlements and an application/container model. It is not
restricted to Mac App Store distribution, but does not provide a drop-in replacement
for UDA's arbitrary server path-list configuration.

For Linux production and macOS development, native builds with Landlock disabled
remain useful for ordinary work. Run the actual Landlock tests in a Linux VM or
container; a successful Seatbelt test is not evidence that the Linux policy works.

## Choosing a deployment approach

For a conventional Linux service, a practical starting point is a dedicated account,
a restricted systemd service, and UDA's Landlock policy for data and scratch paths.
Keep existing AppArmor or SELinux protection if the site uses it. Use a container or
VM where controlling the filesystem view, dependencies or network boundary is also
part of the deployment requirement.

Exercise the deployed combination with representative plugins, certificates and
data mounts. Verify both successful permitted requests and denied access outside
the policy, including writes and execution. When diagnosing a denial, check UDA's
logs, service sandbox settings, mount permissions and host security audit logs:
relaxing a UDA path grant cannot resolve a denial imposed by another layer.

## Testing from Linux or macOS

The permission tests run each policy in a child process so the test runner can clean
up without weakening enforcement. On Linux, they can be built without UDA's other
dependencies:

```sh
cmake -S test/landlock -B /tmp/uda-landlock-tests
cmake --build /tmp/uda-landlock-tests
ctest --test-dir /tmp/uda-landlock-tests --output-on-failure
```

On macOS, run the same tests inside a Linux container (Docker or Podman), for example:

```sh
docker run --rm -v "$PWD:/src:ro" ubuntu:24.04 sh -c '
  apt-get update && apt-get install -y cmake g++ &&
  cmake -S /src/test/landlock -B /tmp/build &&
  cmake --build /tmp/build &&
  ctest --test-dir /tmp/build --output-on-failure'
```

The container VM's kernel must support and enable Landlock ABI 3 or newer; newer
headers inside a container do not upgrade that kernel. Tests fail instead of being
silently skipped if enforcement is unavailable. Full UDA builds with Landlock enabled
also register these tests under `build/test/landlock`, plus a real server/plugin
request test when Python 3, the shared client and the help plugin are available.
Linux CI runs them with enforcement enabled. Builds without Landlock instead register
`Landlock_server_plugin_disabled`, which checks that a configured policy stops the
server from starting; macOS CI runs it.

