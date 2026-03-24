Bricks DSP
-------------------
Cross platform (linux, MacOS and Windows) modular audio dsp block library. Intended as a prototyping tool for synthesizers and audio effects. 

General Concepts
-------------------
BricksDsp is a system for building signal chains at compile time by connecting reasonably high level modules, called Bricks,
together. It could be used as a backend for a dynamic modular synth like Reaktor or Softube Modular as the bricks are at a comparable 
abstraction level to Reaktor. Some work is needed to allow runtime connection in a realtime safe manner, and the processing graph
needs to be managed manually. It's intended primarily as a backend to fixed architecture plugins and as a tool for experimenting.

BricksDsp is with a few exception fully contained in header files and all brick implementations are assumed to be inlined.
Each brick implements a render function that takes single float arguments for control signals that are updated once per block
and audio buffers for input and output. Heavy calculations such as exponentials or trigonometric functions for filters are usually done at control rate but the results are
linearly interpolated at sample rate to avoid volume jumps and glitches.

For efficiency and simplicity BricksDsp uses a fixed audio block size that is set at compile time.
All bricks have a os_factor template parameter that enables that brick to run at x times the block size for oversampled processing. 
Specific bricks can be used as adaptors between oversampled and not oversampled bricks.

The general philosophy in Bricks DSP is to set as many options as possible at compile time rather than at runtime to give
the compiler the best freedom to optimise by inlining and unrolling. Therefore many Bricks have templated options.
Signals

Not to be confused with Brickworks by Orastron, which is a commersial and mature toolkit for building commercial grade stuff, this is just me experimenting. 

-------------------
To stay with common modular concepts and for compatibility with common plugin formats control inputs are assumed to be normalised to a [0, 1] range and [-1, 1] for bipolar inputs. Clipping is done internally only on those bricks where values outside of the nominal range would break things or make filters blow up. Nominal audio levels should also be within [1, -1]
Logarithmic pitch inputs for oscillators, filters, etc, should adhere to 0.1 per octave, essentially the 1v/oct modular standard in 1:10 scale.

Build instructions
-------------------
Create a build directory and call cmake from this directory, as will most CMake projects. Note that there's a few build options settable from CMake. See _CMakeLists.txt_

To include in a CMake based project, add the following to the projects _CMakeLists.txt_ file:
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
Documentation currently mainly consists of inline comments in the code. Also please see the examples for how to use it.

Unit tests and benchmarks
-------------------
Unit tests with gtest are not built and run by default, set the build option __BRICKS_DSP_BUILD_TESTS__ to __ON__ to enable them. The benchmarks are built with gbench and can be enabled using the __BRICKS_DSP_BUILD_BENCHMARKS__ build option. To run the benchmarks type:
````
benchmark/benchmarks
````
Note that benchmark results only make sense on an optimised Release build.

License
-------------------
Released under MIT license, see license file for details.  
Copyright 2018 - 2025 Gustav Andersson. 
Some filter code Copyright 2012 Teemu Voipio
