from __future__ import print_function

from sys import argv, exit, stderr
from os import mkdir, makedirs, chdir, system, getcwd
from os.path import getsize
from re import search
from json import loads
from struct import pack
from errno import EEXIST

if len(argv) < 5:
    print("Usage: bundle.py <info json> <assembly for patches> <build dir> <out dir>", file=stderr)
    exit(1)

info = loads(open(argv[1]).read())
patches_file = open(argv[2]).read()
dir_build = argv[3]
dir_out = argv[4]
dir_top = getcwd()

console_dict = {
    "o3ds": 0,
    "n3ds": 1
}

type_dict = {
    "NATIVE_FIRM": 0,
    "TWL_FIRM": 1,
    "AGB_FIRM": 2
}

options_dict = {
    "keyx": 0b00000001,
    "emunand": 0b00000010,
    "save": 0b00000100
}

for version in info["version_specific"]:
    patches = []
    patch_count = len(info["patches"])
    verdir_build = dir_build + "/" + version["console"] + "-" + version["version"]
    verdir_out = dir_out + "/" + version["console"] + "-" + version["version"]
    verfile = patches_file

    # Create the patches array based on the global and the version-specific array.
    for index in range(patch_count):
        patch = {}
        for array in [info["patches"][index], version["patches"][index]]:
            for patch_info in array:
                patch[patch_info] = array[patch_info]
        patches.append(patch)

    # Set the offset right for the patches
    for patch in patches:
        match = search("(.create.*[\"|']%s[\"|'].*)" % patch["name"], verfile)
        if not match:
            print("Couldn't find where %s is created." % patch["name"], file=stderr)
            exit(1)

        toreplace = match.group(0)
        replaceby = ".create \"%(name)s\", %(offset)s\n.org %(offset)s" % patch
        verfile = verfile.replace(toreplace, replaceby)

    # Set the version-specific variables
    if "variables" in version:
        vartext = ""
        for variable in version["variables"]:
            vartext += ".definelabel %s, %s\n" % (variable, version["variables"][variable])
        verfile = verfile.replace("#!variables\n", vartext)

    # Build dir for this version
    try:
        mkdir(verdir_build)
    except OSError as ex:
        if ex.errno == EEXIST:
            pass
        else:
            raise

    chdir(verdir_build)

    # Compile it
    open("patches.s", "w").write(verfile)
    if system("armips patches.s"):
        print("Couldn't compile version %s for some reason." % version["version"], file=stderr)
        exit(1)

    # Bake the cake
    # What kind of cake is it?
    cake_type = console_dict[version["console"]] << 4 | (type_dict[info["type"]] & 0xF)

    # Create the header
    cake_header = pack("BBBB", patch_count, int(version["version"], 0), cake_type, len(info["description"]) + 5)
    cake_header += (info["description"] + '\0').encode()

    # Create the patch headers
    patch_header_len = 13
    cur_offset = len(cake_header) + patch_header_len * patch_count
    for patch in patches:
        options = 0
        if "options" in patch:
            for option in patch["options"]:
                if option in options_dict:
                    options |= options_dict[option]
                else:
                    print("I don't know what option %s means." % option, file=stderr)
                    exit(1)

        patch_len = getsize(patch["name"])
        cake_header += pack("IIIB", int(patch["offset"], 0), cur_offset, patch_len, options)
        cur_offset += patch_len

    # Append the patches
    cake = cake_header
    for patch in patches:
        cake += open(patch["name"], "rb").read()

    chdir(dir_top)

    try:
        makedirs(verdir_out)
    except OSError as ex:
        if ex.errno == EEXIST:
            pass
        else:
            raise
    open(verdir_out + "/" + info["name"] + ".cake", "wb").write(cake)
