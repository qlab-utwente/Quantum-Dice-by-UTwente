#!/usr/bin/env python3

import json
import os
import shlex
import sys

# These arguments should be removed from the compile commands as these are only for gcc and not for
# clang, so clang does not know what to do with them.
BAD_FLAGS = {
    "-mlongcalls",
    "-fno-tree-switch-conversion",
    "-fstrict-volatile-bitfields",
    "-mdisable-hardware-atomics",
    "-mtext-section-literals",
    "-mfix-esp32-psram-cache-issue",
    "-mbss-section-header",
    "-mno-target-align",
    "-mno-serialize-volatile",
}

# Clang-tidy does not support the esp32 compiler or its target.
# So, we are faking the compiler and the target.
FAKE_COMPILER = "c++"
FAKE_TARGET = "--target=x86_64-pc-linux-gnu"

def cleanArguments(arguments):
    new_arguments = []
    skipNext = False
    for argument in arguments:
        if skipNext:
            skipNext = False
            continue

        # Arguments that start with the @ symbol are files that contain further arguments, we will
        # include those arguments and clean those as well.
        if argument.startswith('@'):
            filePath = argument[1:]
            if os.path.exists(filePath):
                try:
                    with open(filePath, 'r') as f:
                        # Read the arguments from the files and clean the arguments before adding
                        # them to the list of new arguments.
                        fileArguments = shlex.split(f.read())
                        new_arguments.extend(cleanArguments(fileArguments))
                except:
                    print(f"❌ Error: exception caught.")
                    sys.exit(1)
            else:
                print(f"❌ Error: file {filePath} not found.")
                sys.exit(1)
            continue

        # Remove the target.
        if argument.startswith("--target=") or argument.startswith("-target="):
            continue
        if argument == "-target" or argument == "--target":
            skipNext = True
            continue

        # Remove the bad flags.
        if argument in BAD_FLAGS:
            continue

        new_arguments.append(argument)
    return new_arguments

def main():
    # The first argument is the input file.
    # If no arguments are given, then it gives an error.
    if len(sys.argv) == 1:
        print("❌ Error: Not enough arguments.")
        sys.exit(1)
    inputFile = sys.argv[1]
    if not os.path.exists(inputFile):
        print(f"❌ Error: file {inputFile} not found.")
        sys.exit(1)

    # If we have been given two or more arguments, then the second argument will be the output file.
    # If only one argument is given, then the input file is also the output file.
    if len(sys.argv) > 2:
        outputFile = sys.argv[2]
    else:
        outputFile = inputFile
    if not os.path.exists(outputFile):
        print(f"❌ Error: file {outputFile} not found.")
        sys.exit(1)

    # Read the compile commands from the JSON file.
    with open(inputFile, 'r') as f:
        data = json.load(f)

    # Loop over all the compile files in the database.
    # Clean the arguments for each entry.
    for entry in data:
        # Replace the command entry with an argument entry.
        # We can work more easily on the arguments entry as opposed to the arguments.
        if 'command' in entry:
            entry['arguments'] = shlex.split(entry['command'])
            del entry['command']

        # If the argument entry is available, then we will clean the arguments.
        if 'arguments' in entry:
            entry['arguments'] = cleanArguments(entry['arguments'])
            entry['arguments'][0] = FAKE_COMPILER

    # Write the cleaned arguments to the output file.
    with open(outputFile, 'w') as f:
        json.dump(data, f, indent=2)
    print("✔ Done.")

if __name__ == '__main__':
    main()
