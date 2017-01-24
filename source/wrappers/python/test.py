#!/usr/bin/python

import pyidam

pyidam.Client.server = "idam0"
pyidam.Client.port = 56563

client = pyidam.Client()

print 'Fetching signal'
print

try:
	signal = client.get("ip", "13500")
except pyidam.IdamException, e:
	print "Error:", e
	raise SystemExit()

print 'Type:', type(signal)
print 'Name:', signal.label
print 'Units:', signal.units
print 'Rank:', signal.rank
print 'Data:', signal.data
print

for i in range(signal.rank):
    dim = signal.dim(i)

    print 'Dim:', dim.number
    print '  Label:', dim.label
    print '  Units:', dim.units
    print '  Data:', signal.data
    print

signal.plot()

print 'Fetching structured data'
print

try:
	tree = client.get("/", "meta::listData(context=meta, shot=12100, cast=column)")
except pyidam.IdamException, e:
	print 'Error:', e
	raise SystemExit()

print 'Type:', type(tree)
tree.display()

node = tree['ROOT/data']
print 'Name:', node.name
print 'Count:', node.count
