Bricks DSP
-------------------
Modular audio dsp block library. Intended as a prototyping tool for synthesizers and audio effects. 

General Concepts
-------------------
BricksDsp is a system for building signal chains at compile time by connecting reasonably high level modules, called Bricks, together. It's currently not meant as a backend for a dynamic modular synth like Reaktor or Softube Modular, though the bricks are at a comparable abstraction level to Reaktor. But more as a tool for experimenting and possibly as a backend to fixed architecture plugins. Though it will likely be less optimised than a custom architecture.

Connecting several bricks into a signal chain can be made during object instantiating by passing input connections to the brick constructor. Once instantiated, the brick's output ports can be used as inputs to new bricks. Bricks can also be connected to other bricks after creation. A nullpointer can be passed as a "dummy" placeholder to a plugin to connect some inputs during creation, but need to be replaced with a valid Control or Audio input before calling render().

For efficiency and simplicity BricksDsp uses a fixed block size that is set at compile time. Bricks have 2 types of input and output ports. Audio ports are updated every sample and Control ports once for every block. The control rate therefore becomes samplerate / block size. Currently all inputs of a block have to be connected, if an input is not to be used, it must still be connected to a "dummy" source with a fixed value.

The general philosphy in Bricks DSP is to optimise by setting as many options as possible at compile time rather than at runtime to give the compiler the best freedom to optimise. Therefore many Bricks have templated options and simple control-rate Bricks have their render functions in header files for efficient inlining.

Signals
-------------------
To stay with common modular concepts and for compatibility with common plugin formats control inputs are assumed to be normalised to a [0, 1] range and [-1, 1] for bipolar inputs. Clipping is done internally only on those bricks where values outside of the nominal range would break things. Nominal audio levels should also be within [1, -1]
Logarithmic pitch inputs for oscillators, filters, etc, should adhere to 0.1 per octave, essentially the 1v/oct modular standard in 1:10 scale. 

Build instructions
-------------------
The generate script creates a build folder and calls cmake to make a release folder and a debug folder. To rebuild, one can simply cd into build/release or build/debug and do "make".

