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
    
#include "c8080.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define si_swap(a, b, type) \
    do {                    \
        type tmp = (a);     \
        (a) = (b);          \
        (b) = (tmp);        \
    } while (0)

internal inline u16
mem_offset(struct cpu_8080 *cpu)
{
    return (cpu->h << 8) | cpu->l;
}

internal inline u16
hl_u8(u8 high, u8 low)
{
    return (high << 8) | low;
}

internal void
unimplemented_instruction(cpu_8080 *cpu, u8 instruction)
{
    printf("Error: unimplemented instruction: 0x%02x\n", instruction);
    assert(0);
    // exit(1);
}

internal inline u8
parity(u16 f)
{
    int parity = __builtin_popcount(f);
    return (parity & 1) == 0;
}

internal inline void
update_addsub_flags(struct cpu_8080 *cpu, u16 val)
{
    cpu->cc.z = (val & 0xff) == 0;
    cpu->cc.s = (val & 0x80) != 0;
    cpu->cc.cy = val > 0xff;
    cpu->cc.p = parity(val & 0xff);
}

internal inline void
update_increment_flags(struct cpu_8080 *cpu, u8 val)
{
    cpu->cc.z = val == 0;
    cpu->cc.s = (val & 0x80) != 0;
    cpu->cc.p = parity(val);
}

internal inline void
update_logical_flags(struct cpu_8080 *cpu, u8 val)
{
    cpu->cc.z = (val & 0xff) == 0;
    cpu->cc.s = (val & 0x80) != 0;
    cpu->cc.cy = 0;
    cpu->cc.ac = 0;
    cpu->cc.p = parity(val);
}

internal inline void
ret(struct cpu_8080 *cpu)
{
    cpu->pc = ((cpu->m[cpu->sp + 1] << 8) | (cpu->m[cpu->sp + 0]));
    cpu->sp += 2;
}
internal inline void
call(struct cpu_8080 *cpu, u16 addr)
{
    u16 ret = cpu->pc + 2;
    cpu->m[cpu->sp - 1] = (ret >> 8) & 0xff;
    cpu->m[cpu->sp - 2] = (ret & 0xff);
    cpu->pc = addr - 1;
    cpu->sp -= 2;
}

internal inline void
call_hl(struct cpu_8080 *cpu, u8 addrLow, u8 addrHigh)
{
    call(cpu, hl_u8(addrHigh, addrLow));
}

internal inline void
jmp_hl(struct cpu_8080 *cpu, u8 addrLow, u8 addrHigh)
{
    cpu->pc = hl_u8(addrHigh, addrLow) - 1;
}

internal inline void cmp(struct cpu_8080 *cpu, u8 reg0, u8 reg1)
{
    u8 result = reg0 - reg1;
    cpu->cc.z = (reg0 == reg1);
    cpu->cc.cy = (reg0 < reg1);
    cpu->cc.s = (result & 0x80) != 0;
    cpu->cc.p = parity(result);
}

internal inline u8
bitwise_and(struct cpu_8080 *cpu, u8 reg0, u8 reg1)
{
    u8 result = reg0 & reg1;
    update_logical_flags(cpu, result);
    return result;
}

internal inline u8
bitwise_or(struct cpu_8080 *cpu, u8 reg0, u8 reg1)
{
    u8 result = reg0 | reg1;
    update_logical_flags(cpu, result);
    return result;
}

internal inline u8
bitwise_xor(struct cpu_8080 *cpu, u8 reg0, u8 reg1)
{
    u8 result = reg0 ^ reg1;
    update_logical_flags(cpu, result);
    return result;
}

internal inline u8
increment(struct cpu_8080 *cpu, u8 val)
{
    u8 result = val + 1;
    update_increment_flags(cpu, result);
    return result;
}

internal inline u8
decrement(struct cpu_8080 *cpu, u8 val)
{
    u8 result = val - 1;
    update_increment_flags(cpu, result);
    return result;
}

internal inline u8
add(struct cpu_8080 *cpu, u8 val0, u8 val1)
{
    u16 result = (u16)val0 + (u16)val1;
    update_addsub_flags(cpu, result);
    return result & 0xff;
}

internal inline u8
sub(struct cpu_8080 *cpu, u8 val0, u8 val1)
{
    u16 result = (u16)val0 - (u16)val1;
    update_addsub_flags(cpu, result);
    return result & 0xff;
}

internal inline u8
carry_add(struct cpu_8080 *cpu, u8 val0, u8 val1)
{
    u16 result = (u16)val0 + (u16)val1 + (u16)cpu->cc.cy;
    update_addsub_flags(cpu, result);
    return result & 0xff;
}

internal inline u8
carry_sub(struct cpu_8080 *cpu, u8 val0, u8 val1)
{
    u16 result = (u16)val0 - (u16)val1 - (u16)cpu->cc.cy;
    update_addsub_flags(cpu, result);
    return result & 0xff;
}

internal inline void
stack_push_u16(struct cpu_8080 *cpu, u16 val)
{
    cpu->m[cpu->sp - 1] = (val & 0xff00) >> 8;
    cpu->m[cpu->sp - 2] = (val & 0xff);
    cpu->sp -= 2;
}

internal inline void
stack_push_u8_hl(struct cpu_8080 *cpu, u8 high, u8 low)
{
    cpu->m[cpu->sp - 1] = high;
    cpu->m[cpu->sp - 2] = low;
    cpu->sp -= 2;
}

internal inline void
stack_pop_u8_hl(struct cpu_8080 *cpu, u8 *regH, u8 *regL)
{
    *regL = cpu->m[cpu->sp + 0];
    *regH = cpu->m[cpu->sp + 1];
    cpu->sp += 2;
}

internal inline void
dad(struct cpu_8080 *cpu, u8 regH, u8 regL)
{
    u16 hl = ((u16)cpu->h << 8) | cpu->l;
    u16 ab = ((u16)regH << 8) | regL;
    u32 result = hl + ab;
    cpu->cc.cy = (result > 0xffff);
    cpu->h = (result & 0xff00) >> 8;
    cpu->l = result & 0xff;
}

internal inline void
generate_interrupt(struct cpu_8080 *cpu, i32 interruptNum)
{
    stack_push_u16(cpu, cpu->pc);
    cpu->pc = 8 * interruptNum; // RST [interrupt number]
}

internal void
emulate_8080(struct cpu_8080 *cpu)
{
    u8 *oc = &cpu->m[cpu->pc];

    // clang-format off
    switch (*oc) {

        case 0x00: break; // NOP
        case 0x08: break; // NOP
        case 0x10: break; // NOP
        case 0x18: break; // NOP
        case 0x20: break; // NOP
        case 0x28: break; // NOP
        case 0x30: break; // NOP
        case 0x38: break; // NOP
        case 0xcb: break; // NOP
        case 0xd9: break; // NOP
        case 0xdd: break; // NOP
        case 0xed: break; // NOP
        case 0xfd: break; // NOP

        case 0x02: { //STAX B
            cpu->m[hl_u8(cpu->b, cpu->c)] = cpu->a;
        } break;
        case 0x12: { //STAX D
            cpu->m[hl_u8(cpu->d, cpu->e)] = cpu->a;
            u16 addr = (cpu->d << 8) | cpu->e;
            cpu->m[addr] = cpu->a;
        } break;
        case 0x32: { //STA addr 
            cpu->m[hl_u8(oc[2], oc[1])] = cpu->a;
            cpu->pc += 2;
        } break;

        case 0x0a: { //LDAX B
            u16 addr = hl_u8(cpu->b, cpu->c);
            cpu->a = cpu->m[addr];
        } break;
        case 0x1a: { //LDAX D
            u16 addr = hl_u8(cpu->d, cpu->e);
            cpu->a = cpu->m[addr];
        } break;
        case 0x3a: { //LDA addr 
            u16 addr = hl_u8(oc[2], oc[1]);
            cpu->a = cpu->m[addr];
            cpu->pc += 2;
        } break;

        case 0x22: { //SHLD addr
            u16 addr = hl_u8(oc[2], oc[1]);
            cpu->m[addr+0] = cpu->l;
            cpu->m[addr+1] = cpu->h;
            cpu->pc += 2;
        } break;

        case 0x2a: { //LHLD addr
            u16 addr = hl_u8(oc[2], oc[1]);
            cpu->l = cpu->m[addr+0];
            cpu->h = cpu->m[addr+1];
            cpu->pc += 2;
        } break;

        case 0x07: {  //RLC (A = A << 1; bit 0 = prev bit 7; CY = prev bit 7)
            u8 result = cpu->a << 1;
            u8 bit7 = cpu->a >> 7;
            cpu->cc.cy = bit7;
            cpu->a = result | bit7; 
        } break;

        case 0x0f: { //RRC (A = A >> 1; bit 7 = prev bit 0; CY = prev bit 0)
            //shift
            u8 result = cpu->a >> 1;
            u8 bit0 = (cpu->a & 0x01);
            cpu->cc.cy = bit0;
            cpu->a = result | (bit0 << 7);
        } break;

        case 0x17: { //RAL (A = A << 1; bit0 = prev CY; CY = prev bit 7)
            u8 result = (cpu->a << 1) | cpu->cc.cy;
            cpu->cc.cy = cpu->a >> 7;
            cpu->a = result;
        } break;

        case 0x1f: { //RAR (A = A >> 1; bit 7 = prev bit 7; CY = prev bit 0)
            u8 result = cpu->a >> 1;
            u8 bit7 = cpu->a & 0x80;
            cpu->cc.cy = cpu->a & 0x01; 
            cpu->a = result | bit7;
        } break;

        case 0x2f: /* CMA */ cpu->a = ~cpu->a; break; 
        case 0x3f: /* CMC */ cpu->cc.cy = !cpu->cc.cy; break;
        case 0x37: /* STC */ cpu->cc.cy = 1; break;

        case 0x01: { // LXI B,word
            cpu->c = oc[1];
            cpu->b = oc[2];
            cpu->pc += 2;
        } break;
        case 0x11: { // LXI D,word
            cpu->e = oc[1];
            cpu->d = oc[2];
            cpu->pc += 2;
        } break;
        case 0x21: { // LXI H,word
            cpu->l = oc[1];
            cpu->h = oc[2];
            cpu->pc += 2;
        } break;
        case 0x31: { // LXI SP,word
            cpu->sp = hl_u8(oc[2], oc[1]);
            cpu->pc += 2;
        } break;

        case 0x03: { // INX B
            u16 bc = hl_u8(cpu->b, cpu->c);
            ++bc;
            cpu->b = bc >> 8;
            cpu->c = bc & 0xff;
        } break;
        case 0x13: { // INX D
            u16 de = hl_u8(cpu->d, cpu->e);
            ++de;
            cpu->d = de >> 8;
            cpu->e = de & 0xff;
        } break;
        case 0x23: { // INX H
            u16 hl = hl_u8(cpu->h, cpu->l);
            ++hl;
            cpu->h = hl >> 8;
            cpu->l = hl & 0xff;
        } break;
        case 0x33: { // INX SP
            ++cpu->sp;
        } break;

        case 0x0b:  { // DCX B 
            u16 bc = ((u16)cpu->b << 8) | (u16)cpu->c;
            --bc;
            cpu->b = bc >> 8;
            cpu->c = bc & 0xff;
        } break;
        case 0x1b:  { // DCX D
            u16 de = ((u16)cpu->d << 8) | (u16)cpu->e;
            --de;
            cpu->d = de >> 8;
            cpu->e = de & 0xff;
        } break;
        case 0x2b:  { // DCX H
            u16 hl = ((u16)cpu->h << 8) | (u16)cpu->l;
            --hl;
            cpu->h = hl >> 8;
            cpu->l = hl & 0xff;
        } break;
        case 0x3b:  { // DCX SP 
            --cpu->sp;
        } break;

        case 0x09: /* DAD B */ dad(cpu, cpu->b, cpu->c);  break;
        case 0x19: /* DAD D */ dad(cpu, cpu->d, cpu->e);  break;
        case 0x29: /* DAD H */ dad(cpu, cpu->h, cpu->l);  break;

        case 0x39: { // DAD SP 
            u16 hl = ((u16)cpu->h << 8) | (u16)cpu->l;
            u32 result = hl + cpu->sp;
            cpu->cc.cy = (result > 0xff);
            cpu->h = (result & 0xffff) >> 8;
            cpu->l = result & 0xff;
        } break;
        
        //MVI 
        case 0x06: cpu->b = oc[1]; cpu->pc++; break;
        case 0x0e: cpu->c = oc[1]; cpu->pc++; break;
        case 0x16: cpu->d = oc[1]; cpu->pc++; break;
        case 0x1e: cpu->e = oc[1]; cpu->pc++; break;
        case 0x26: cpu->h = oc[1]; cpu->pc++; break;
        case 0x2e: cpu->l = oc[1]; cpu->pc++; break;
        case 0x36: cpu->m[mem_offset(cpu)] = oc[1]; cpu->pc++; break;
        case 0x3e: cpu->a = oc[1]; cpu->pc++; break;

        // INR 
        case 0x04: cpu->b = increment(cpu, cpu->b); break;
        case 0x0c: cpu->c = increment(cpu, cpu->c); break;
        case 0x14: cpu->d = increment(cpu, cpu->d); break;
        case 0x1c: cpu->e = increment(cpu, cpu->e); break;
        case 0x24: cpu->h = increment(cpu, cpu->h); break;
        case 0x2c: cpu->l = increment(cpu, cpu->l); break;
        case 0x34: cpu->m[mem_offset(cpu)] = increment(cpu, cpu->m[mem_offset(cpu)]); break;
        case 0x3c: cpu->a = increment(cpu, cpu->a); break;

        // DCR
        case 0x05: cpu->b = decrement(cpu, cpu->b); break;
        case 0x0d: cpu->c = decrement(cpu, cpu->c); break;
        case 0x15: cpu->d = decrement(cpu, cpu->d); break;
        case 0x1d: cpu->e = decrement(cpu, cpu->e); break;
        case 0x25: cpu->h = decrement(cpu, cpu->h); break;
        case 0x2d: cpu->l = decrement(cpu, cpu->l); break;
        case 0x35: cpu->m[mem_offset(cpu)] = decrement(cpu, cpu->m[mem_offset(cpu)]); break;
        case 0x3d: cpu->a = decrement(cpu, cpu->a); break;

        //MOV B,R
        case 0x40 ... 0x45: cpu->b = cpu->r[*oc-0x40+1]; break;
        case 0x46: cpu->b = cpu->m[mem_offset(cpu)]; break; 
        case 0x47: cpu->b = cpu->a; break;

        //MOV C,R
        case 0x48 ... 0x4d: cpu->c = cpu->r[*oc-0x48+1]; break;
        case 0x4e: cpu->c = cpu->m[mem_offset(cpu)]; break; 
        case 0x4f: cpu->c = cpu->a; break;

        //MOV D,R
        case 0x50 ... 0x55: cpu->d = cpu->r[*oc-0x50+1]; break;
        case 0x56: cpu->d = cpu->m[mem_offset(cpu)]; break; 
        case 0x57: cpu->d = cpu->a; break;

        //MOV E,R
        case 0x58 ... 0x5d: cpu->e = cpu->r[*oc-0x58+1]; break;
        case 0x5e: cpu->e = cpu->m[mem_offset(cpu)]; break; 
        case 0x5f: cpu->e = cpu->a; break;

        //MOV H,R
        case 0x60 ... 0x65: cpu->h = cpu->r[*oc-0x60+1]; break;
        case 0x66: cpu->h = cpu->m[mem_offset(cpu)]; break; 
        case 0x67: cpu->h = cpu->a; break;

        //MOV L,R
        case 0x68 ... 0x6d: cpu->l = cpu->r[*oc-0x68+1]; break;
        case 0x6e: cpu->l = cpu->m[mem_offset(cpu)]; break; 
        case 0x6f: cpu->l = cpu->a; break;

        //MOV M,R
        case 0x70 ... 0x75: cpu->m[mem_offset(cpu)] = cpu->r[*oc-0x70+1]; break;
        case 0x77: cpu->m[mem_offset(cpu)] = cpu->a; break;

        //MOV A,R
        case 0x78 ... 0x7d: cpu->a = cpu->r[*oc-0x78+1]; break;
        case 0x7e: cpu->a = cpu->m[mem_offset(cpu)]; break; 
        case 0x7f: cpu->a = cpu->a; break;

        //ADD
        case 0x80 ... 0x85: cpu->a = add(cpu, cpu->a, cpu->r[*oc-0x80+1]); break;
        case 0x86: cpu->a = add(cpu, cpu->a, cpu->m[mem_offset(cpu)]); break; 
        case 0x87: cpu->a = add(cpu, cpu->a, cpu->a); break; 

        //ADC
        case 0x88 ... 0x8d: cpu->a = carry_add(cpu, cpu->a, cpu->r[*oc-0x88+1]); break;
        case 0x8e: cpu->a = carry_add(cpu, cpu->a, cpu->m[mem_offset(cpu)]); break;
        case 0x8f: cpu->a = carry_add(cpu, cpu->a, cpu->a); break;

        //SUB
        case 0x90 ... 0x95: cpu->a = sub(cpu, cpu->a, cpu->r[*oc-0x90+1]); break;
        case 0x96: cpu->a = sub(cpu, cpu->a, cpu->m[mem_offset(cpu)]); break; 
        case 0x97: cpu->a = sub(cpu, cpu->a, cpu->a); break; 

        //SBB
        case 0x98 ... 0x9d: cpu->a = carry_sub(cpu, cpu->a, cpu->r[*oc-0x98+1]); break;
        case 0x9e: cpu->a = carry_sub(cpu, cpu->a, cpu->m[mem_offset(cpu)]); break; 
        case 0x9f: cpu->a = carry_sub(cpu, cpu->a, cpu->a); break; 

        //ANA 
        case 0xa0 ... 0xa5: cpu->a = bitwise_and(cpu, cpu->a, cpu->r[*oc-0xa0+1]); break;
        case 0xa6: cpu->a = bitwise_and(cpu, cpu->a, cpu->m[mem_offset(cpu)]); break; 
        case 0xa7: cpu->a = bitwise_and(cpu, cpu->a, cpu->a); break; 

        //XRA 
        case 0xa8 ... 0xad: cpu->a = bitwise_xor(cpu, cpu->a, cpu->r[*oc-0xa8+1]); break;
        case 0xae: cpu->a = bitwise_xor(cpu, cpu->a, cpu->m[mem_offset(cpu)]); break; 
        case 0xaf: cpu->a = bitwise_xor(cpu, cpu->a, cpu->a); break; 

        //CMP
        case 0xb8 ... 0xbd: cmp(cpu, cpu->a, cpu->r[*oc-0xb8+1]); break;
        case 0xbe: cmp(cpu, cpu->a, cpu->m[mem_offset(cpu)]); break; 
        case 0xbf: cmp(cpu, cpu->a, cpu->a); break; 

        //ORA 
        case 0xb0 ... 0xb5: cpu->a = bitwise_or(cpu, cpu->a, cpu->r[*oc-0xb0+1]); break;
        case 0xb6: cpu->a = bitwise_or(cpu, cpu->a, cpu->m[mem_offset(cpu)]); break; 
        case 0xb7: cpu->a = bitwise_or(cpu, cpu->a, cpu->a); break; 

        case 0xc6: /* ADI */ cpu->a = add(cpu, cpu->a, oc[1]);         cpu->pc++; break; 
        case 0xce: /* ACI */ cpu->a = carry_add(cpu, cpu->a, oc[1]);   cpu->pc++; break; 
        case 0xd6: /* SUI */ cpu->a = sub(cpu, cpu->a, oc[1]);         cpu->pc++; break; 
        case 0xde: /* SBI */ cpu->a = carry_sub(cpu, cpu->a, oc[1]);   cpu->pc++; break; 
        case 0xe6: /* ANI */ cpu->a = bitwise_and(cpu, cpu->a, oc[1]); cpu->pc++; break;
        case 0xee: /* XRI */ cpu->a = bitwise_xor(cpu, cpu->a, oc[1]); cpu->pc++; break;
        case 0xf6: /* ORI */ cpu->a = bitwise_or(cpu, cpu->a, oc[1]);  cpu->pc++; break;
        case 0xfe: /* CPI */ cmp(cpu, cpu->a, oc[1]);                    cpu->pc++; break;

        case 0xc1: /* POP B */  stack_pop_u8_hl(cpu, &cpu->b, &cpu->c); break;
        case 0xd1: /* POP D */  stack_pop_u8_hl(cpu, &cpu->d, &cpu->e); break;
        case 0xe1: /* POP H */  stack_pop_u8_hl(cpu, &cpu->h, &cpu->l); break;

        case 0xc5: /* PUSH B */ stack_push_u8_hl(cpu, cpu->b, cpu->c); break;
        case 0xd5: /* PUSH D */ stack_push_u8_hl(cpu, cpu->d, cpu->e); break;
        case 0xe5: /* PUSH H */ stack_push_u8_hl(cpu, cpu->h, cpu->l); break;

        case 0xf1: { // POP PSW
            u8 flags = cpu->m[cpu->sp + 0];
            cpu->cc.cy = (flags >> 0) & 1;
            cpu->cc.p  = (flags >> 2) & 1;
            cpu->cc.ac = (flags >> 4) & 1;
            cpu->cc.z  = (flags >> 6) & 1;
            cpu->cc.s  = (flags >> 7) & 1;
            cpu->a  = cpu->m[cpu->sp + 1];
            cpu->sp += 2;
        } break;

        case 0xf5: { // PUSH PSW
            u8 flags = cpu->cc.cy;
            flags |= 1 << 1;
            flags |= cpu->cc.p  << 2;
            flags |= cpu->cc.ac << 4;
            flags |= cpu->cc.z  << 6;
            flags |= cpu->cc.s  << 7;
            cpu->m[cpu->sp - 2] = flags;
            cpu->m[cpu->sp - 1] = cpu->a;
            cpu->sp -= 2;
        } break;

        case 0xe3: { //XHTL
            si_swap(cpu->l, cpu->m[cpu->sp + 0], u8);
            si_swap(cpu->h, cpu->m[cpu->sp + 1], u8);
        } break;
        case 0xeb: { //XCHG
            si_swap(cpu->h, cpu->d, u8);
            si_swap(cpu->l, cpu->e, u8);
        } break;
 
        case 0xc9: /* RET */ ret(cpu); break; 
        case 0xc0: /* RNZ */ if(cpu->cc.z  == 0) ret(cpu); break; 
        case 0xc8: /* RZ  */ if(cpu->cc.z  == 1) ret(cpu); break; 
        case 0xd0: /* RNC */ if(cpu->cc.cy == 0) ret(cpu); break; 
        case 0xd8: /* RC  */ if(cpu->cc.cy == 1) ret(cpu); break;
        case 0xe0: /* RPO */ if(cpu->cc.p  == 0) ret(cpu); break;
        case 0xe8: /* RPE */ if(cpu->cc.p  == 1) ret(cpu); break;
        case 0xf0: /* RP  */ if(cpu->cc.s  == 0) ret(cpu); break; 
        case 0xf8: /* RM  */ if(cpu->cc.s  == 1) ret(cpu); break;
            
        case 0xcd: /* CALL */ { 
#if 1 //NOTE: this is specific to the cpu-diag program
            if(((oc[2] << 8) | oc[1]) == 5) {
                if(cpu->c == 9) {
                    uint16_t offset = (cpu->d << 8) | (cpu->e);    
                    char *str = &cpu->m[offset+3];  //skip the prefix bytes    
                    while (*str != '$') {
                        printf("%c", *str++);    
                    }
                    printf("\n");    
                    assert(false);
                } else if (cpu->c == 2) {    
                    printf ("print char routine called\n");    
                }  
            } else if (((oc[2] << 8) | oc[1]) == 0) {
                assert(false);
            } else {
                call_hl(cpu, oc[1], oc[2]); break;
            }    
#else
            call_hl(cpu, oc[1], oc[2]); 
#endif
        } break;

        case 0xc4: /* CNZ  */ if(cpu->cc.z  == 0) call_hl(cpu, oc[1], oc[2]); else cpu->pc += 2; break;
        case 0xcc: /* CZ   */ if(cpu->cc.z  == 1) call_hl(cpu, oc[1], oc[2]); else cpu->pc += 2; break;
        case 0xd4: /* CNC  */ if(cpu->cc.cy == 0) call_hl(cpu, oc[1], oc[2]); else cpu->pc += 2; break;
        case 0xdc: /* CC   */ if(cpu->cc.cy == 1) call_hl(cpu, oc[1], oc[2]); else cpu->pc += 2; break;
        case 0xe4: /* CPO  */ if(cpu->cc.p  == 0) call_hl(cpu, oc[1], oc[2]); else cpu->pc += 2; break;
        case 0xec: /* CPE  */ if(cpu->cc.p  == 1) call_hl(cpu, oc[1], oc[2]); else cpu->pc += 2; break;
        case 0xf4: /* CP   */ if(cpu->cc.s  == 0) call_hl(cpu, oc[1], oc[2]); else cpu->pc += 2; break;
        case 0xfc: /* CM   */ if(cpu->cc.s  == 1) call_hl(cpu, oc[1], oc[2]); else cpu->pc += 2; break;

        case 0xc3: /* JMP */ jmp_hl(cpu, oc[1], oc[2]); break;
        case 0xc2: /* JNZ */ if(cpu->cc.z  == 0) jmp_hl(cpu, oc[1], oc[2]); else cpu->pc += 2; break;
        case 0xca: /* JZ  */ if(cpu->cc.z  == 1) jmp_hl(cpu, oc[1], oc[2]); else cpu->pc += 2; break;
        case 0xd2: /* JNC */ if(cpu->cc.cy == 0) jmp_hl(cpu, oc[1], oc[2]); else cpu->pc += 2; break;
        case 0xda: /* JC  */ if(cpu->cc.cy == 1) jmp_hl(cpu, oc[1], oc[2]); else cpu->pc += 2; break;
        case 0xe2: /* JPO */ if(cpu->cc.p  == 0) jmp_hl(cpu, oc[1], oc[2]); else cpu->pc += 2; break;
        case 0xea: /* JPE */ if(cpu->cc.p  == 1) jmp_hl(cpu, oc[1], oc[2]); else cpu->pc += 2; break;
        case 0xf2: /* JP  */ if(cpu->cc.s  == 0) jmp_hl(cpu, oc[1], oc[2]); else cpu->pc += 2; break;
        case 0xfa: /* JM  */ if(cpu->cc.s  == 1) jmp_hl(cpu, oc[1], oc[2]); else cpu->pc += 2; break;

        case 0xf9: /* SPHL */ cpu->sp = (cpu->h << 8) | cpu->l; break; 
        case 0xe9: /* PCHL */ cpu->pc = (cpu->h << 8) | cpu->l; cpu->pc--; break;

        case 0xc7: /* RST 0 */ call(cpu,  0); break;
        case 0xcf: /* RST 1 */ call(cpu,  8); break;
        case 0xd7: /* RST 2 */ call(cpu, 10); break;
        case 0xdf: /* RST 3 */ call(cpu, 18); break;
        case 0xe7: /* RST 4 */ call(cpu, 20); break;
        case 0xef: /* RST 5 */ call(cpu, 28); break;
        case 0xf7: /* RST 6 */ call(cpu, 30); break;
        case 0xff: /* RST 7 */ call(cpu, 38); break;

        case 0x27: /* DAA */ break; // Special
//        case 0xd3: /* OUT */ break; // Special
//       case 0xdb: /* IN  */ break; // Special

        case 0xf3: /* DI  */ cpu->interruptEnabled = 0; break;
        case 0xfb: /* EI  */ cpu->interruptEnabled = 1; break;

        case 0x76: exit(1); break; //HLT(special)

        default: {
            unimplemented_instruction(cpu, oc[0]);
            //printf("Unimplemented: 0x%02x\n", *oc);
        } break;
    }
    cpu->pc += 1;
    // clang-format on
}
