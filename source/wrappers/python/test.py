#!/usr/local/bin/python3

#   #!/usr/bin/python

import pyuda

pyuda.Client.server = "idam0"
pyuda.Client.port = 56563

client = pyuda.Client()

print('Fetching signal')
print()

try:
	signal = client.get("ip", "13500")
except pyuda.UDAException as e:
	print("Error:", e)
	raise SystemExit()

print('Type:', type(signal))
print('Name:', signal.label)
print('Units:', signal.units)
print('Rank:', signal.rank)
print('Data:', signal.data)
print()

for i in range(signal.rank):
    dim = signal.dim(i)

    print('Dim:', dim.number)
    print('  Label:', dim.label)
    print('  Units:', dim.units)
    print('  Data:', signal.data)
    print()

signal.plot()

print('Fetching structured data')
print()

try:
	tree = client.get("/", "meta::listData(context=meta, shot=12100, cast=column)")
except pyuda.UDAException as e:
	print('Error:', e)
	raise SystemExit()

print('Type:', type(tree))
tree.display()

node = tree['ROOT/data']
print('Name:', node.name)
print('Count:', node.count)
