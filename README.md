Bricks DSP
-------------------
Cross platform (linux, MacOS and Windows) modular audio dsp block library. Intended as a prototyping tool for synthesizers and audio effects. 

General Concepts
-------------------
BricksDsp is a system for building signal chains at compile time or run time by connecting reasonably high level modules, called Bricks, together. It could be used as a backend for a dynamic modular synth like Reaktor or Softube Modular, the bricks are at a comparable abstraction level to Reaktor. Some care needs to be taken to allow runtime connection in a realtime safe manner, and the processing graph needs to be managed manually. It's intended more as a tool for experimenting and possibly as a backend to fixed architecture plugins.

Connecting several bricks into a signal chain can be made during object instantiating by passing input connections to the brick constructor. Once instantiated, the brick's output ports can be used as inputs to new bricks. Bricks can also be connected to other bricks after creation. A nullpointer can be passed as a "dummy" placeholder to a plugin to connect some inputs during creation, but need to be replaced with a valid Control or Audio input before calling render().

For efficiency and simplicity BricksDsp uses a fixed audio block size that is set at compile time. Bricks have 2 types of input and output ports. Audio ports are updated every sample and Control ports once for every block. The control rate therefore becomes samplerate / block size. Currently all inputs of a block have to be connected, if an input is not to be used, it must still be connected to a "dummy" source with a fixed value.

The general philosophy in Bricks DSP is to enable setting as many options as possible at compile time rather than at runtime to give the compiler the best freedom to optimise. Therefore many Bricks have templated options and simple control-rate Bricks have their render functions in header files for efficient inlining.

Signals
-------------------
To stay with common modular concepts and for compatibility with common plugin formats control inputs are assumed to be normalised to a [0, 1] range and [-1, 1] for bipolar inputs. Clipping is done internally only on those bricks where values outside of the nominal range would break things or make filters blow up. Nominal audio levels should also be within [1, -1]
Logarithmic pitch inputs for oscillators, filters, etc, should adhere to 0.1 per octave, essentially the 1v/oct modular standard in 1:10 scale.

Build instructions
-------------------
Create a build directory and call cmake from this directory, as will most CMake projects. Note that there's a few build options settable from CMake. See _CMakeLists.txt_

To include in a CMake based project, add the following to the projects _CMakeLists.txt_
````
add_subdirectory(BrickDsp)
target_link_libraries(target_name bricks_dsp)
````
And in your code
````
#include "bricks_dsp/bricks.h"
````
The examples are configured by default but not added to the _all_ target. To build the examples type:
````
make synth_voice
make fixture
make jack_fixture (linux only)
````

Documentation
-------------------
Documentation currently consists of inline comments in the code. Also see the examples for how to use it.

Unit tests and benchmarks
-------------------
Unit tests with gtest are not built and run by default, set the build option __BRICKS_DSP_BUILD_TESTS__ to __ON__ to enable them. The benchmarks are built with gbench and can be enabled using the __BRICKS_DSP_BUILD_BENCHMARKS__ build option. To run the benchmarks type:
````
benchmark/benchmarks
````
Note that benchmarks only make sense in an optimised Release build.

License
-------------------
Released under MIT license, see license file for details.  
Copyright 2018 - 2025 Gustav Andersson. 
Some filter code Copyright 2012 Teemu Voipio
