#!/usr/bin/python3
from sys import argv
from struct import unpack_from, calcsize

header_struct = "<BBB"
patch_struct = "<B8sIIBBIBI"
version_struct = "<III"
subtype_struct = "<HHI"

cake = open(argv[1], "rb").read()

version, patch_count, patch_offset = unpack_from(header_struct, cake, 0)
print("Format version: %i" % version)
print("Amount of patches: %i" % patch_count)
print("Offset of patch headers: %i" % patch_offset)

for patch_index in range(patch_count):
    patch_type, subtype, code_offset, size, options, version_count, versions_offset, variable_count, variables_offset = unpack_from(patch_struct, cake, patch_offset + calcsize(patch_struct) * patch_index)
    print("Type: %i" % patch_type)
    print("Subtype: %s" % subtype)
