Template for new projects.
==========================
Includes CMake configuration (with Google Test and third-party libraries support), easy access to log library and command line options parsing.

Quick instructions:
 * rename the CMake project from `project_template` to your prefered name
 * grep for `projecttemplate` and `project_template` (case insensitive) in `main.cpp` and `options.h`
   and change those to your project name
 * take a look at how command line arguments are passed and how to use debug macros

Directory layout:
---------------

```
include/        : public header files should be put in this directory
misc/           : stuff that is not directly related to the build targets, i.e. accessory scripts to generate configuration data, etc.
src/            : All source files and corresponding header files should go here. For larger projects the source files should be placed in folders separated by logical units within the project.
test/           : testing code
    unittests/  : All unit test source code goes here, the directory structure should mirror that of the src dir and the files names should be xxxx_test.cpp.
    gtest-1.7/  : The gtest framework. Automatically built with the tests.
    tools/      : Examples and non-unit tests for e.g. third-party libraries.
third_party/    : Any third party dependencies that are to be compiled into the project goes here
```

Build instructions:
-------------------
The generate script creates a build folder and calls cmake to make a release folder and a debug folder. To rebuild, one can simply cd into build/release or build/debug and do "make".

