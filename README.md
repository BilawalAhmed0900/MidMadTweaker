# MidMadTweaker

Unlock cars in Midtown Madness(1999)

# Usage

    Usage: MidMadTweaker.exe [-h] Input Output

    Positional Arguments:
      Input: Input file to read from
      Output: Output file to write to

    Optional Arguments:
      -h: Show this help and exit

    Note: Input and Output can both be same...
 
Use it on UI.ar file like this: 
`MidMadTweaker.exe UI.ar UI.ar`

# Compilation
    g++.exe -m32 -o MidMadTweaker_x86 -static -Ofast -Wall -Werror -std=c++0x MidMadTweaker.cpp
    g++.exe -m64 -o MidMadTweaker_x64 -static -Ofast -Wall -Werror -std=c++0x MidMadTweaker.cpp
