# Host-side G-code parser test

This test compiles the same `GCodeParser.h` header used by the TFT firmware. It checks
missing command numbers, valid M/G/T identifiers, the `T-1` form, and numeric overflow.

Run from the repository root with GCC (for example, TDM-GCC on Windows):

```powershell
gcc -std=c99 -Wall -Wextra -Werror -pedantic -I TFT\src\User\API tests\host\test_gcode_parser.c -o tests\host\test_gcode_parser.exe
.\tests\host\test_gcode_parser.exe
```

The test is independent of the TFT hardware and PlatformIO firmware environment.
