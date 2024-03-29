# c8080
Bare 8080 cpu emulator. Based on this guide http://www.emulator101.com/welcome.html

Currently does not support DAA(Decimal Adjust Accumulator) instructions.

This repo doesn't include any machine layers.

### Planned features:
- DAA support
- Cycle stepping instead of instruction stepping for better compatibility with other hardware emulation.

### Testing
The code was tested using the cpudiag progam found in the test folder. 

Compile test.c with:
`clang test.c -DCPUDIAG=1` 
or `gcc test.c -DCPUDIAG=1`

When you run it you should see `CPU IS OPERATIONAL`


Note: the cpudiag code uses a platform specific instruction `ORG 00100H` to start the program at byte 0x100.
To deal with this [test.c](https://github.com/Sir-Irk/c8080/blob/bd9e242ad73db7ae3c7343e605eeb6a002eb4431/test/test.c#L58) does a little trick to make it work.

There is also a special version of [CALL(0xcd)](https://github.com/Sir-Irk/c8080/blob/48cfecebc5079d6b22f81234cc32750625f2017e/c8080.c#L585) in the emulator for handling the printing for the diagnostic program. This needs to be enabled with `-DCPUDIAG=1` when you compile for testing.
