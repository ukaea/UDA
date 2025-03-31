#!python3

import pytoml
import argparse

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-i", "--input_file", type=argparse.FileType('r'), required=True)
arg_parser.add_argument("-o", "--output_file", type=argparse.FileType('w'), required=True)
arg_parser.add_argument("-s", "--section", type=str, required=True)
arg_parser.add_argument("-n", "--name", type=str, required=True)
arg_parser.add_argument("-v", "--value", type=str, required=True)
args = arg_parser.parse_args()

toml = pytoml.load(args.input_file)
args.input_file.close()

if args.value in ('true', 'True'):
    toml[args.section][args.name] = True
elif args.value in ('false', 'False'):
    toml[args.section][args.name] = False
else:
    try:
        toml[args.section][args.name] = int(args.value)
    except ValueError:
        toml[args.section][args.name] = args.value

pytoml.dump(toml, args.output_file)
args.output_file.close()