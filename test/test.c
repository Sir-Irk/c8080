#include <stdio.h>
#include <string.h>
#include "../c8080.c"


static void
print_state(cpu_8080 *c)
{
    printf("PC: %hu | SP: %hu | IE: %u\n", c->pc, c->sp, c->interruptEnabled);
    printf("A: %u | B: %u | C: %u | D: %u | E %u | H %u | L %u\n", c->a, c->b, c->c, c->d, c->e, c->h, c->l);
    printf("Z: %u | S: %u | P: %u | CY: %u | AC: %u\n\n", c->cc.z, c->cc.s, c->cc.p, c->cc.cy, c->cc.ac);
}

typedef struct read_file_result {
    size_t contentsSize;
    void     *contents;
} read_file_result;

//NOTE: Don't use this janky thing outside of here.
internal struct read_file_result
read_entire_file(const char *filepath)
{
    struct read_file_result result = {};

    FILE *f = fopen(filepath, "rb");
    assert(f);

    size_t size = 0;
    fseek(f, 0, SEEK_END);
    size = ftell(f) + 1;
    fseek(f, 0, SEEK_SET); // rewind
    void *contents = malloc(size);
    assert(contents);
    fread(contents, 1, size - 1, f);
    fclose(f);
    ((char *)contents)[size - 1] = '\0';

    result.contentsSize = size;
    result.contents     = contents;

    return result;
}

int main(void)
{
    struct read_file_result program = read_entire_file("cpudiag.bin");
    assert(program.contents);

    struct cpu_8080 cpu = {};
    u16 stackSize = 5000;

    cpu.m = malloc(program.contentsSize + stackSize + 0x100);
    memcpy(cpu.m + 0x100, program.contents, program.contentsSize);

    cpu.pc = 0x100;

    // Fix the first instruction to be JMP 0x100
    cpu.m[0] = 0xc3;
    cpu.m[1] = 0x65;
    cpu.m[2] = 0x00;

    // Fix the stack pointer from 0x6ad to 0x7ad
    //  this 0x06 byte 112 in the code, which is
    //  byte 112 + 0x100 = 368 in memory
    cpu.m[368] = 0x7;
    // Skip DAA test
    cpu.m[0x59c] = 0xc3; // JMP
    cpu.m[0x59d] = 0xc2; // addr byte 1
    cpu.m[0x59e] = 0x05; // addr byte 2

    while (emulate_8080(&cpu)) {
        //print_state(&cpu);
    }

    return 0;
}