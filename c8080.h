/*
MIT License

Copyright (c) 2022 Jeremy Montgomery

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef C8080_INCLUDE_GUARD
#define C8080_INCLUDE_GUARD

#include <stdint.h>

#define internal static
#define local_persist static

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct condition_codes {
    u8 z : 1;
    u8 s : 1;
    u8 p : 1;
    u8 cy : 1;
    u8 ac : 1;
    u8 pad : 3;
} condition_codes;

typedef struct cpu_8080 {
    union {
        struct {
            u8 a, b, c, d, e, h, l;
        };
        u8 r[7];
    };

    u16 sp;
    u16 pc;
    u8 *m;
    condition_codes cc;
    u8 interruptEnabled;
} cpu_8080;

//Returns 0 when exit is called. Returns 1 otherwise
int emulate_8080(struct cpu_8080 *cpu);

#endif //C8080_INCLUDE_GUARD
