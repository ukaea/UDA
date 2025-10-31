#!/usr/bin/env python3
"""
Generate a versioned EasyBuild .eb file from a Jinja2 template.

Usage:
    python scripts/generate_eb.py --version 2.8.0 --checksum d5e8b3... > UDA-2.8.0.eb
"""

import argparse
import datetime
from jinja2 import Environment, FileSystemLoader
from pathlib import Path
import hashlib

# Default values
DEFAULT_TOOLCHAIN = "13.2.0"
DEFAULT_PYTHON_VERSION = "3.11.5"
DEFAULT_BUILD_TYPE = "Release"
DEFAULT_PARALLEL = 4
UDA_TEMPLATE_FILE = "uda.eb.j2"
PYUDA_TEMPLATE_FILE = "pyuda.eb.j2"


def get_source_url(version):
    if '.' in version:
        return "https://github.com/ukaea/UDA/archive/refs/tags"
    else:
        return "https://github.com/ukaea/UDA/archive/"


def calculate_checksum_from_url(url):
    """Optional helper to compute SHA256 checksum if not provided and file exists."""
    try:
        import requests
        r = requests.get(url, timeout=30)
        r.raise_for_status()
        checksum = hashlib.sha256(r.content).hexdigest()
        return checksum
    except Exception as e:
        print(f"Warning: could not fetch {url} to compute checksum: {e}")
        return "UNKNOWN"


def main():
    parser = argparse.ArgumentParser(description="Generate an EasyBuild .eb file from a template")
    parser.add_argument("--version", required=True, help="Version string (e.g., 2.8.0 or main)")
    parser.add_argument("--toolchain-version", default=DEFAULT_TOOLCHAIN)
    parser.add_argument("--checksum", help="SHA256 checksum of source archive")
    parser.add_argument("--git-sha", help="git sha for this version")
    parser.add_argument("--build-type", default=DEFAULT_BUILD_TYPE)
    parser.add_argument("--python_version", default=DEFAULT_PYTHON_VERSION)
    parser.add_argument("--parallel", type=int, default=DEFAULT_PARALLEL)
    parser.add_argument("--template-dir", default="easybuild/templates")
    parser.add_argument("--output-dir", default="easybuild/generated")
    parser.add_argument("--uda_template_file", default=UDA_TEMPLATE_FILE)
    parser.add_argument("--pyuda_template_file", default=PYUDA_TEMPLATE_FILE)

    args = parser.parse_args()

    # Ensure output directory exists
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    if args.git_sha:
        version = args.git_sha
        url = get_source_url(args.git_sha)
    else:
        version = args.version
        url = get_source_url(args.version)

    # Compute checksum if missing
    if args.checksum:
        checksum = args.checksum
    else:
        checksum = calculate_checksum_from_url(f"{url}/{version}.zip")

    env = Environment(loader=FileSystemLoader(args.template_dir), trim_blocks=True, lstrip_blocks=True)

    uda_template = env.get_template(args.uda_template_file)
    rendered = uda_template.render(
        version=args.version,
        checksum=checksum,
        source_url=url,
        source_file=f"{version}.zip",
        toolchain_version=args.toolchain_version,
        build_type=args.build_type,
        parallel=args.parallel,
        date=datetime.date.today().isoformat(),
    )
    filename = f"uda-{args.version}.eb"
    out_path = output_dir / filename
    out_path.write_text(rendered)
    print(f"Generated {out_path}")

    pyuda_template = env.get_template(args.pyuda_template_file)
    rendered = pyuda_template.render(
        version=args.version,
        toolchain_version=args.toolchain_version,
        python_version=args.python_version,
        date=datetime.date.today().isoformat(),
    )
    filename = f"pyuda-{args.version}.eb"
    out_path = output_dir / filename
    out_path.write_text(rendered)
    print(f"Generated {out_path}")


if __name__ == "__main__":
    main()
