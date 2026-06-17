#!/bin/env python3

import re
import json
from collections import namedtuple
import argparse

RE_STRING = r'(?P<host>\d+\.\d+\.\d+\.\d+) - (?P<uid>[\w\d_\-]+)? \[(?P<datetime>[a-zA-Z0-9: ]+)\] \[(?P<req_id>\d+) (?P<signal>.+) (?P<exp_no>-?\d+) (?P<pass>-?\d+)  (?P<path>[\w\d\-_\.\/+]+)? (?P<file>[\w\d\-_\.\/+]+)? (?P<format>[a-zA-Z_]+)? (?P<archive>[a-zA-Z_]+)? (?P<device>[a-zA-Z_]+)? \] (?P<err_code>-?\d+) (?P<db_size>\d+) \[(?P<err_msg>.*)\] (?P<elapsed_time>\d+(?:\.\d*(?:e[+\-]?\d+)?)?) (?P<client_ver>\d+) (?P<server_ver>\d+) \[(?P<client_pid>\d+) (?P<server_pid>\d+)\] \[\]'

log_regex = re.compile(RE_STRING)

LogLine = namedtuple('LogLine',
    ['host', 'uid', 'datetime', 'req_id', 'signal', 'exp_no', 'pass_no', 'path',
     'file', 'format', 'archive', 'device', 'err_code', 'db_size', 'err_msg',
     'elapsed_time', 'client_ver', 'server_ver', 'client_pid', 'server_pid'])

line_start_regex = re.compile(r'^\d+\.\d+\.\d+\.\d+ - ')


def log_line_generator(lines) -> (int, str):
    """
    Generator which joins log entries split by spurious newline
    characters, yielding full lines and the starting line-number of
    that entry in the original file
    """
    buffer = []
    start_line_num = None

    for i, line in enumerate(lines):
        if line_start_regex.match(line):
            if buffer:
                yield start_line_num, ''.join(buffer)
                buffer = []
            start_line_num = i
        buffer.append(line.rstrip('\r\n'))
    if buffer:
        yield start_line_num, ''.join(buffer)


def apply_filters(log, filters):
    if filters is None:
        return True
    for key, val in filters:
        if str(getattr(log, key)) != val:
            return False
    return True


def parsed_entry_generator(logfile, filters=None):
    if filters is None:
        filter_vals = []
    else:
        filter_vals = [entry[1] for entry in filters]

    with open(logfile) as file:
        for i, line in log_line_generator(file):
            if filter_vals and not all(v in line for v in filter_vals):
                continue
            match = log_regex.match(line)
            if not match:
                print(f'failed to parse log line {i + 1}:\n{line}', file=sys.stderr)
                continue
            groups = match.groups()
            log_line = LogLine(*groups)
            if not apply_filters(log_line, filters):
                continue
            yield log_line


def write_to_json(line_generator, outfile):
    with open(outfile, "w") as f:
        f.write("[\n")
        first = True

        for entry in line_generator:
            if not first:
                f.write(",\n")
            json.dump(entry._asdict(), f, indent=2)
            first = False

        f.write("\n]\n")


def parse_filter_arg(s):
    if "=" not in s:
        raise argparse.ArgumentTypeError("Filters must be key=value")
    key, value = s.split("=", 1)
    return key.strip(), value.strip()


if __name__ == '__main__':
    import sys
    parser = argparse.ArgumentParser()
    parser.add_argument("input_file", help="the log file to parse")
    parser.add_argument("--output", help="optional json file to write output to")
    parser.add_argument("--filter", action="append", type=parse_filter_arg)

    args = parser.parse_args()
    entries = parsed_entry_generator(args.input_file, args.filter)
    if args.output is not None:
        write_to_json(entries, args.output)

