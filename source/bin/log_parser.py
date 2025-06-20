#!/bin/env python3

import re
import json
from collections import namedtuple

RE_STRING = r'(?P<host>\d+\.\d+\.\d+\.\d+) - (?P<uid>[\w\d_\-]+)? \[(?P<datetime>[a-zA-Z0-9: ]+)\] \[(?P<req_id>\d+) (?P<signal>.+) (?P<exp_no>\d+) (?P<pass>-?\d+)  (?P<path>[\w\d\-_\.\/+]+)? (?P<file>[\w\d\-_\.\/+]+)? (?P<format>[a-zA-Z_]+)? (?P<archive>[a-zA-Z_]+)? (?P<device>[a-zA-Z_]+)? \] (?P<err_code>\d+) (?P<db_size>\d+) \[(?P<err_msg>.*)\] (?P<elapsed_time>\d+(?:\.\d*(?:e[+\-]?\d+)?)?) (?P<client_ver>\d+) (?P<server_ver>\d+) \[(?P<client_pid>\d+) (?P<server_pid>\d+)\] \[\]'

log_regex = re.compile(RE_STRING)

LogLine = namedtuple('LogLine',
    ['host', 'uid', 'datetime', 'req_id', 'signal', 'exp_no', 'pass_no', 'path',
     'file', 'format', 'archive', 'device', 'err_code', 'db_size', 'err_msg',
     'elapsed_time', 'client_ver', 'server_ver', 'client_pid', 'server_pid'])


def parse(logfile, outfile):
    lines = []
    with open(logfile) as file:
        for i, line in enumerate(file):
            match = log_regex.match(line)
            if not match:
                print(f'failed to parse log line {i + 1}:\n{line}', file=sys.stderr)
                continue
            groups = match.groups()
            log_line = LogLine(*groups)
            lines.append(log_line._asdict())
    with open(outfile, 'w') as file:
        json.dump(lines, file, indent=2)


if __name__ == '__main__':
    import sys

    args = sys.argv
    if len(args) != 3:
        print(f'usage: {args[0]} logfile outfile', file=sys.stderr)
        raise SystemExit()

    logfile = args[1]
    outfile = args[2]

    parse(logfile, outfile)
