import xml.etree.cElementTree as ET

tree = ET.parse('IDSDef.xml')
root = tree.getroot()

def walk(node):
    if 'name' not in node.attrib: return []
    name = node.attrib['name']
    array = False
    values = []
    if node.tag != 'IDS' and 'maxoccur' in node.attrib and (node.attrib['maxoccur'] == 'unbounded' or int(node.attrib['maxoccur']) > 1):
        array = True
    for child in node:
        values += walk(child)
    if values:
        if array:
            return ([(name + '/Shape_of', 'static')]
                    + [(name + '/#/' + n, type) for (n, type) in values])
        else:
            return [(name + '/' + n, type) for (n, type) in values]
    else:
        return [(name, node.attrib['type'])]

values = []
version = ''
for IDS in root:
    version = max(IDS.attrib['lifecycle_version'], version)
    values += walk(IDS)

file_name = 'IMAS_mapping_' + version + '.xml'

with open(file_name, 'w') as file:
    print('<?xml version="1.0" encoding="UTF-8"?>', file=file)
    print('<mappings IDS_version="{0}">'.format(version), file=file)
    for value in values:
        print('<mapping key="{0}" value="" type="{1}"/>'.format(*value), file=file)
    print('</mappings>', file=file)
