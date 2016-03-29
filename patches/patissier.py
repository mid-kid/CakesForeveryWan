from sys import argv, stderr, exit
from struct import pack, calcsize
from yaml import load, dump
from os.path import isfile

# If LibYAML is available, use that, as recommended by the PyYAML wiki.
try:
    from yaml import CLoader as Loader, CDumper as Dumper
except ImportError:
    from yaml import Loader, Dumper

# Globals
format_version = 1
header_struct = "<BBB"
patch_struct = "<B8sIIBBIBI"
version_struct = "<III"
subtype_struct = "<HHI"
patch_types = {
    "FIRM": 0,
    "Memory": 1,
    "Userland": 2,
    "Sysmodule": 3
}
firm_types = {
    "NATIVE_FIRM": 0,
    "TWL_FIRM": 1,
    "AGB_FIRM": 2
}
consoles_dict = {
    "o3ds": 0,
    "n3ds": 1
}
options_dict = {
    "keyx": 0b00000001,
    "emunand": 0b00000010,
    "save": 0b00000100
}

# Shitty function to kill itself
def die(string):
    print(string, file=stderr)
    exit(1)

if len(argv) < 3:
    die("Usage: %s <info.yaml> <output.cake>" % argv[0])

try:
    info = load(open(argv[1]), Loader=Loader)
except Exception as e:
    print(e)
    die("Failed to load the YAML file: %s" % argv[1])

if not "description" in info:
    die("Missing description in info")
if not "patches" in info:
    die("Missing patches in info")

# Open destination file
cake = open(argv[2], "wb")

# Main header
cake.write(pack(header_struct,
    format_version,
    len(info["patches"]),
    len(info["description"]) + calcsize(header_struct) + 1
))
cake.write((info["description"] + '\0').encode())

# Know the starting positions of the patches array and the versions array
patches_offset = cake.tell()
versions_offset = cake.tell() + calcsize(patch_struct) * len(info["patches"])

# We'll save all the known memory patches here
memory_list = []

for patch_name in info["patches"]:
    patch = info["patches"][patch_name]

    # Read the code.
    # NOTE: Loading into memory due to being a lazy asshole, and I don't expect too big files. If it ends up being used for considerably big files, though, consider using mmap.
    try:
        patch_file = open(patch_name, "rb")
    except FileNotFoundError:
        die("Couldn't find patch file: %s" % patch_name)
    patch_code = patch_file.read()
    patch_size = patch_file.tell()
    patch_file.close()

    if not isinstance(patch, dict):
        die("Incompatible type for patch: %s" % patch_name)
    if not "type" in patch:
        die("Missing type in patch: %s" % patch_name)
    if not isinstance(patch["type"], str):
        die("Incompatible type for type in patch: %s" % patch_name)
    if not "versions" in patch:
        die("Missing versions in patch: %s" % patch_name)
    if not isinstance(patch["versions"], dict):
        die("Incompatible type for versions in patch: %s" % patch_name)

    # Figure out how many versions there are, and do some basic checks.
    if patch["type"] == "Userland":
        # Userland patches don't have console-specific versions
        version_count = len(patch["versions"])
    else:
        # Memory and FIRM patches, and Sysmodules, however, do have console-specific versions.
        version_count = 0
        for console in patch["versions"]:
            if not console in consoles_dict:
                die("Unknown console: %s" % console)
            if not isinstance(patch["versions"][console], dict):
                die("Incompatible type for console: %s" % console)

            version_count += len(patch["versions"][console])

    # Figure out the type and subtype values
    type = None
    subtype = None
    if patch["type"] in firm_types:
        # We support using type: firm_type as a shorthand for type: firm and subtype: firm_type
        type = "FIRM"
        subtype = patch["type"]
    else:
        if not patch["type"] in patch_types:
            die("Unknown type in patch: %s" % patch_name)
        if not "subtype" in patch:
            die("Missing subtype in patch: %s" % patch_name)

        type = patch["type"]
        subtype = patch["subtype"]

    # Convert subtype into the appropiate value
    if type == "Userland":
        # Userland patches use the title ID
        if not isinstance(subtype, int):
            die("Incompatible subtype in patch: %s" % patch_name)
    else:
        # Most other patches have a bit more complicated structure.
        if not subtype in firm_types:
            die("Unknown subtype in patch: %s" % patch_name)

        memory_id = 0
        memory_var = 0
        memory_name = None

        # Fix the memory ID for Memory and FIRM patches
        if type == "FIRM" and "memory" in patch:
            # If a FIRM patch requires a memory patch, set the info for it right.
            if not isinstance(patch["memory"], dict):
                die("Incompatible type for memory in patch: %s" % patch_name)
            if not "patch" in patch["memory"]:
                die("Missing patch in memory in patch: %s" % patch_name)
            if not "variable" in patch["memory"]:
                die("Missing variable in memory in patch: %s" % patch_name)

            memory_name = patch["memory"]["patch"]

            # A FIRM patch also needs to know the location of the variable to write the Memory patch's location to
            if not isinstance(patch["memory"]["variable"], str):
                die("Incompatible type for variable in memory in patch: %s" % patch_name)
            memory_var = patch_code.find(patch["memory"]["variable"].encode())
            if memory_var == -1:
                die("Coudn't find variable '%s' in patch: %s" % (patch["memory"]["variable"], patch_name))
        elif type == "Memory":
            # Memory patches themselves are indexed by default
            memory_name = patch_name

        if memory_name:
            if not memory_name in memory_list:
                memory_list.append(memory_name)

            memory_id = memory_list.index(memory_name) + 1  # +1 because 0 means it has no ID.

        subtype = pack(subtype_struct,
            firm_types[subtype],
            memory_id,
            memory_var
        )

    # Type is converted the same way in either case
    type = patch_types[type]

    # Get options
    options = 0
    if "options" in patch:
        if not isinstance(patch["options"], list):
            die("Incompatible type for options in patch: %s" % patch_name)

        for option in patch["options"]:
            if not option in options_dict:
                die("Unrecognized option: %s" % option)
            options |= options_dict[option]

    # We will put the variables arrays behind the versions array.
    variables_offset = versions_offset + calcsize(version_struct) * version_count

    # Make the variables array
    variables = 0
    variable_count = 0
    if "variables" in patch:
        if not isinstance(patch["variables"], list):
            die("Incompatible type for variables in patch: %s" % patch_name)

        # Jump to the place where we will write the variables array
        cake.seek(variables_offset)
        variables = cake.tell()
        for variable in patch["variables"]:
            if not isinstance(variable, str):
                die("Incompatible type for variable in patch: %s" % patch_name)

            offset = patch_code.find(variable.encode())
            if offset == -1:
                die("Coudn't find variable '%s' in patch: %s" % (variable, patch_name))
            cake.write(pack("I", offset))

            variable_count += 1

        # Save the location of the variable values for future use
        variables_offset = cake.tell()

    # Set the start of the versions array correctly
    versions = versions_offset

    # Process all different versions
    for version in patch["versions"]:
        if patch["type"] == "Userland":
            pass
            # Userland patches don't have console-specific versions
            # TODO: Implement this.
        else:
            # Memory and FIRM patches, and Sysmodules, however, do have console-specific versions.
            console = version
            for version in patch["versions"][console]:
                version_info = patch["versions"][console][version]

                identifier = consoles_dict[console] << 16 | version

                variables_info = None
                if isinstance(version_info, int):
                    # We support writing the offset without any hassle if there's no variables to specify.
                    memory_offset = version_info
                elif isinstance(version_info, list) and patch["type"] == "Memory":
                    # We also support writing just the variables (FIRM patches require also specifying the offset)
                    memory_offset = 0
                    variables_info = version_info
                elif isinstance(version_info, dict):
                    if not "offset" in version_info:
                        die("Missing offset in version: %s-%s-%x" % (patch_name, console, version))
                    if not isinstance(version_info["offset"], int):
                        die("Incompatible type for offset in version: %s-%s-%x" % (patch_name, console, version))
                    if "variables" in version_info:
                        if not isinstance(version_info["variables"], list):
                            die("Incompatible type for variables in version: %s-%s-%x" % (patch_name, console, version))

                        variables_info = version_info["variables"]

                    memory_offset = version_info["offset"]
                else:
                    die("Incompatible type for version: %s-%s-%x" % (patch_name, console, version))

                # Process the variables
                version_variables = 0
                if "variables" in patch:
                    if not variables_info:
                        die("Missing variables in version: %s-%s-%x" % (patch_name, console, version))
                    if len(variables_info) != variable_count:
                        die("Incorrect amount of variables in version: %s-%s-%x" % (patch_name, console, version))

                    version_variables = variables_offset
                    cake.seek(variables_offset)
                    for variable in variables_info:
                        if not isinstance(variable, int):
                            die("Incompatible type for variable in version: %s-%s-%x" % (patch_name, console, version))

                        cake.write(pack("I", variable))
                    variables_offset = cake.tell()

                cake.seek(versions_offset)
                cake.write(pack(version_struct,
                    identifier,
                    memory_offset,
                    version_variables
                ))
                versions_offset = cake.tell()

    # Skip over the variable arrays we put behind the versions array
    cake.seek(variables_offset)

    # Write the actual code to the file
    cake.write(b'\0' * (4 - cake.tell() % 4))  # Align to 4 bytes
    patch_offset = cake.tell()  # The current location is the start of the patch
    cake.write(patch_code)

    # Set the current location for writing the next versions array.
    versions_offset = cake.tell()

    # Close the patch file
    patch_file.close()

    # Write the actual patch header.
    cake.seek(patches_offset)
    cake.write(pack(patch_struct,
        type,
        subtype,
        patch_offset,
        patch_size,
        options,
        version_count,
        versions,
        variable_count,
        variables
    ))
    patches_offset = cake.tell()

cake.close()
