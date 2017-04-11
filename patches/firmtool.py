#!/usr/bin/env python3

"""
Simple tool that can help reading FIRM binaries' headers.

It also has functionality to scan for offsets needed for various patches in this repo,
 via the search_native option.
"""

from sys import argv, stderr, exit
from os.path import isfile
from struct import unpack_from
from binascii import unhexlify

if len(argv) <= 1:
    print("Usage: %s <firm> [options]" % argv[0], file=stderr)
    exit(1)

if not isfile(argv[1]):
    print("Can't find file:", argv[1], file=stderr)
    exit(2)

firm = open(argv[1], "rb").read()

sections = []

# Unpack all sections
for x in range(3):
    struct = unpack_from("<IIII32B", firm, 0x40 + 0x30 * x)

    # If size is 0, section doesn't exist
    if not struct[2]:
        continue

    stype = "Unknown"
    if struct[3] == 0:
        stype = "ARM9"
    elif struct[3] == 1:
        stype = "ARM11"

    sections.append({
        "offset": struct[0],
        "address": struct[1],
        "size": struct[2],
        "type": stype,
        "hash": struct[4:]
    })

# Look for process9
for section in sections:
    for x in range(section["size"] - 8):
        if unpack_from("8s", firm, section["offset"] + x)[0] == b"Process9":
            offset = section["offset"] + x

            if unpack_from("4s", firm, offset - 0x100)[0] == b"NCCH":
                section = {}
                section["address"] = unpack_from("I", firm, offset + 0x10)[0]
                section["size"] = unpack_from("I", firm, offset + 0x800 + 12)[0]
                section["offset"] = offset + 0x800 + 0x200
                section["type"] = "Process9"

                # Process9 has preference when searching for things
                sections = [section] + sections

                break

for section in sections:
    print("Section:")
    print("\tOffset: 0x%08X" % section["offset"])
    print("\tAddress: 0x%08X" % section["address"])
    print("\tSize: 0x%08X" % section["size"])

    if "hash" in section:
        print("\tHash: {", end='')
        for x in section["hash"][:0x10]:
            print("0x%02X, " % x, end='')
        print("\b \b\b}")
print()

def tofile(address):
    for section in sections:
        if address >= section["address"] and address < section["address"] + section["size"]:
            return section["offset"] + (address - section["address"])

def toram(offset):
    for section in sections:
        if offset >= section["offset"] and offset < section["offset"] + section["size"]:
            return section["address"] + (offset - section["offset"])

def search(pattern):
    if type(pattern) == str:
        pattern = unhexlify(pattern.replace(" ", ""))

    offset = firm.find(pattern)

    if not offset:
        return "Not found"

    if firm.find(pattern, offset + 1) != -1:
        return "Multiple matches"

    return offset

def simple_search(name, pattern, off=0):
    offset = search(pattern)

    if type(offset) == str:
        print("%s: %s" % (name, offset))
        return

    address = toram(offset)

    if not address:
        print("%s: Not found in any section" % name)
        return

    print("%s: 0x%08X" % (name, address + off))

def decode_bl(instruction, offset):
    return (offset + 8 - (-((instruction & 0xFFFFFF) << 2) & (0xFFFFFF << 2)))

if len(argv) >= 2:
    if argv[2] == "tofile":
        if len(argv) >= 3:
            offset = tofile(int(argv[3], 0))
            if not offset:
                print("Not found in any section")
            else:
                print("Offset found: 0x%08X" % offset)
    if argv[2] == "toram":
        if len(argv) >= 3:
            address = toram(int(argv[3], 0))
            if not address:
                print("Not found in any section")
            else:
                print("Address found: 0x%08X" % address)

    if argv[2] == "search_native":
        print("Signatures:")
        simple_search("\tpatch1", "C0 1C 76 E7")
        simple_search("\tpatch2", "0C 00 69 68 CE B0", -4)

        print("Emunand:")
        off = search("21 20 18 20")
        if type(off) == str:
            print("\tsdmmc: %s" % off)
        else:
            print("\tsdmmc: 0x%08X" % (unpack_from("<I", firm, off + 9)[0] + unpack_from("<I", firm, off + 13)[0]))
        simple_search("\tpatch2", "03 00 24 00 00 00 10 10")
        simple_search("\tpatch3", "10 BD FE B5 04 00 0D 00 17 00 1E 00", 4)
        simple_search("\tpatch4", "FE BD FE B5 04 00 0D 00 17 00 1E 00", 4)

        print("Reboot:")
        off = search("E2 20 20 90")
        if type(off) == str:
            print("\tpatch1: %s" % off)
        else:
            off -= 47

            patch1 = toram(off)
            if not patch1:
                print("\tpatch1: Not found in any section")
            else:
                print("\tpatch1: 0x%08X" % patch1)

            fopen = toram(decode_bl(unpack_from("<I", firm, off + 28)[0], off + 28))
            if not fopen:
                print("\tfopen: Not found in any section")
            else:
                print("\tfopen: 0x%08X" % fopen)

        off = search("00 20 A0 E3 02 30 A0 E1 02 10 A0 E1 02 00 A0 E1")
        if type(off) == str:
            print("\trebc: %s" % off)
        else:
            print("\trebc: 0x%08X" % unpack_from("<I", firm, off + 84)[0])

        simple_search("\tpatch2", "F0 4F 2D E9 3C D0 4D E2 10 0F 12 EE 30 CF 12 EE")

        print("Firmprot:")
        simple_search("\tpatch1", "C4 17 64 1C 98 98", -16)

        print("Slot0x25keyX:")
        simple_search("\tsetkey", "D1 68 01 61 91 68 41 61 51 68 81 61 11 68", -38)
        simple_search("\tunk", "10 B5 02 48 01 F0 7E FF  10 BD")
        simple_search("\tpatch2", "22 00 05 21 28 00 2E")
