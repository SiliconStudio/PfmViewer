# PFM/PHM image viewer desktop application

PFM stands for Portable FloatMap Image Format, as specified by https://www.pauldebevec.com/Research/HDR/PFM/
PHM is the half-float variant as used by intel open image denoise library.

This software is a minimalist (563 C++/ispc lines) desktop app made with Nana++ GUI library.  
It makes use of auto-vectorizing compiler ispc to exploit superscalar AVX512 instructions for tone/exposure updates.  
It is built with subsystem:console on windows so it can accept piped data to stdin.

![screen capture](sshot.png)

## Usage:

one argument for file name (or optionally - (dash) to indicate stdin)
If no arguments: it will probe for stdin pending data, or open a filebox dialog picker if no pending data on stdin.

## Create .pfm

Use that snippet to output (usually linear space) 16bits-float-RGB (half3) images from your software:

```cpp
    // export to PHM:
    std::ofstream f{"hdrimg.phm", std::ios::out | std::ios::binary};
    f << "PH\n";   // capital H = 3 channels of halfs
    f << resolutionX << " " << resolutionY << "\n";
    f << "-1.0\n";  // scale and endian (neg=little)
    f.write((const char*)rowmajorHalfRgbImageData, resolutionX * resolutionY * 3 * 2);
```
magic legend:  
`PF`=`float3`  
`Pf`=`float`  
`PH`=`half3`  
`Ph`=`half`

## Build:

 - install ispc (version 1.18.0 was used for dev) https://ispc.github.io/downloads.html
if you have a different version (or unorthodox install path), tune the CMakeLists.txt to adjust the fallback path `set (ISPC_EXECUTABLE "$ENV{ProgramFiles}/ISPC/ispc-v1.18.0-windows/bin/ispc.exe")` to your version and actual installation path.
 - install cmake 3.15 or newer
 - Download ninja-win.zip from https://github.com/ninja-build/ninja/releases and put ninja.exe in a PATH accessible folder.
 - decompress the nana.zip in place, so that you have a "nana" folder which immediately contains the .hpp.
 The release .lib for visual studio toolset 142 is included in the zip
 - open a "x64 visual studio tool console" (or execute vcvars64.bat)  
then call  
`cmake_suggestion_ninja_cl.bat`
 - cd to build/
 - call ninja

You should have `PfmViewer.exe` ready to go

## Debug:

If you want to build in debug mode,

Go to https://github.com/cnjinhao/nana get the 1.7.4 version, build it yourself using your prefered project system.  
Copy the .lib produced to nana/BIN

edit `cmake_suggestion_ninja_cl.bat` and change `-DCMAKE_BUILD_TYPE=Release` to `-DCMAKE_BUILD_TYPE=Debug`
