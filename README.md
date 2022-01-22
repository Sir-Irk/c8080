# c8080
Bare 8080 cpu emulator. Based on this guide http://www.emulator101.com/welcome.html

Currently does not support DAA(Decimal Adjust Accumulator) instructions.

This repo doesn't include any machine layers.

### Testing
code was tested using the following cpu diagnostic program made for the 8080

- Source code : http://www.emulator101.com/files/cpudiag.asm
- Compiled program : http://www.emulator101.com/files/cpudiag.bin

Note: the test code uses a machine specific thing to start the program at byte 0x100.
To deal with this you can use something like the following to make it work(or edit hex manually)

```C
    //...load the entire program with a 0x100 byte offset

    //Fix the first instruction to be JMP 0x100    
    cpu->m[0]=0xc3;    
    cpu->m[1]=0;    
    cpu->m[2]=0x01;    

    //Fix the stack pointer from 0x6ad to 0x7ad    
    // this 0x06 byte 112 in the code, which is    
    // byte 112 + 0x100 = 368 in memory    
    cpu->m[368] = 0x7;    

    //Skip DAA test    
    cpu->m[0x59c] = 0xc3; //JMP    
    cpu->m[0x59d] = 0xc2;    
    cpu->m[0x59e] = 0x05;    
```

There is also a specific instruction(0xcd) in the emulator for handling the printing for the diagnostic program.
