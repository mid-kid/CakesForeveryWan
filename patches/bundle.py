from sys import argv, exit, stderr
from os import mkdir, makedirs, chdir, system, getcwd
from os.path import getsize
from re import search
from json import loads
from struct import pack

if len(argv) < 5:
    print("Usage: bundle.py <info json> <assembly for patches> <build dir> <out dir>", file=stderr)
    exit(1)

# TODO: Define version-specific variables in info.json

info = loads(open(argv[1]).read())
patches_file = open(argv[2]).read()
dir_build = argv[3]
dir_out = argv[4]
dir_top = getcwd()

for version in info["version_specific"]:
    patches = []
    patch_count = len(info["patches"])
    ver = version["version"]
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
            print("Couldn't find where %s is created, skipping..." % patch["name"], file=stderr)
            continue

        toreplace = match.group(0)
        replaceby = ".create \"%(name)s\", %(offset)s\n.org %(offset)s" % patch
        verfile = verfile.replace(toreplace, replaceby)

    # Build dir for this version
    try:
        mkdir(dir_build + "/" + ver)
    except FileExistsError:
        pass

    chdir(dir_build + "/" + ver)

    # Compile it
    open("patches.s", "w").write(verfile)
    if system("armips patches.s"):
        print("Couldn't compile version %s for some reason." % ver, file=stderr)
        exit(1)

    # Bake the cake
    # Create the header
    cake_header = pack("BBxB", patch_count, int(ver, 0), len(info["description"]) + 5)
    cake_header += (info["description"] + '\0').encode()

    # Create the patch headers
    patch_header_len = 13
    cur_offset = len(cake_header) + patch_header_len * patch_count
    for patch in patches:
        patch_len = getsize(patch["name"])
        # TODO: Parse options
        options = patch["options"] if "options" in patch else 0
        cake_header += pack("IIIB", int(patch["offset"], 0), cur_offset, patch_len, options)
        cur_offset += patch_len

    # Append the patches
    cake = cake_header
    for patch in patches:
        cake += open(patch["name"], "rb").read()

    chdir(dir_top)

    verdir = dir_out + "/" + ver
    try:
        makedirs(verdir)
    except FileExistsError:
        pass
    open(verdir + "/" + info["name"] + ".cake", "wb").write(cake)
