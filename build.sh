#!/bin/bash

# gcc fluidsynth.c `pkg-config --cflags --libs lua fluidsynth` -shared -o cfluidsynth.so

# Build with:
gcc -O0 -g fluidsynth.c -undefined dynamic_lookup -I/usr/local/Cellar/fluid-synth/1.1.6/include -I/usr/local/include -L/usr/local/Cellar/fluid-synth/1.1.6/lib -L/usr/local/lib -lfluidsynth -lm -shared -o test/cfluidsynth.so

# Debug with:
#    lldb -- lua -e 'require "cfluidsynth"'

# Test with:
#    lua -e 'require "cfluidsynth"'
