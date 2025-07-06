# Lyra-Samples

This is a library where I created some sample programs as a collection to indicate the features **Lyra-Engine** supports.
I am adding a README.md to every sample to indicate what's special about this sample. I personally take this repository
as some sort of documentation and tutorial for anyone who intends to use **Lyra-Engine**.

Originally this repository intends to be a part of **Lyra-Engine** repository. However, as the sample becomes more complex,
we are expecting more assets style files to be committed into the repo. This will adds unnecssary files and pollute the
engine repository. Therefore I decided to move the samples repository to its own space.

## Build

In order to build samples, [Lyra-Engine](https://github.com/Lyra-Engine/Lyra-Engine) has to be built and installed in
advance. Users could follow the build and installation guide in **Lyra-Engine**, and then build this project using the
following commands:

```bash
cmake -S . -B Scratch           # configure CMake, or alternatively
cmake -S . -B Scratch -A x64    # configure CMake (explicitly with x64)
cmake --build Scratch           # build with CMake
```

## Collections

1. Window (an example how to get user inputs)
2. Triangle (an example of the most basic graphics pipeline)

## Author(s)

[Tianyu Cheng](tianyu.cheng@utexas.edu)
