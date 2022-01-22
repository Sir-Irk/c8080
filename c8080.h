
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

typedef struct machine_8080 {
    u8 shift[2];
    u8 shiftOffset;
    u8 ports[8];
} machine_8080;
