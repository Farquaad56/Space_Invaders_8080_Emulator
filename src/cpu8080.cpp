#include "cpu8080.hpp"
#include "alu.hpp"
#include <iostream>

CPU8080::CPU8080() 
    : regs{0, 0, 0, 0, 0, 0, 0}, pc(0), sp(0), flagsByte(0), 
      flags(flagsByte), halted(false), inte(false), 
      interruptPending(false), intLatch(-1), totalCycles(0), 
      currentInstructionTStates(0) 
{
    reset();
}

void CPU8080::reset() {
    regs = {0, 0, 0, 0, 0, 0, 0};
    pc = 0x0000;
    sp = 0x23FF; // Space Invaders standard
    flagsByte = 0;
    flags.setByte(0);
    halted = false;
    inte = false;
    interruptPending = false;
    intLatch = -1;
    totalCycles = 0;
    currentInstructionTStates = 0;
    initInstructions();
}

void CPU8080::initInstructions() {
    for(int i=0; i<256; ++i) instructions[i] = nullptr;

    // ==================== FLAGS INSTRUCTIONS ====================
    instructions[0x37] = [](CPU8080& cpu) -> uint8_t {
        cpu.flags.setCarry(true);
        cpu.currentInstructionTStates = 4;
        return 1;
    };
    instructions[0x3F] = [](CPU8080& cpu) -> uint8_t {
        cpu.flags.setCarry(!cpu.flags.isCarry());
        cpu.currentInstructionTStates = 4;
        return 1;
    };

    // ==================== SINGLE REGISTER / MEMORY INSTRUCTIONS ====================
    instructions[0x00] = [](CPU8080& cpu) -> uint8_t { 
        cpu.currentInstructionTStates = 4;
        return 1; 
    };
    instructions[0x2F] = [](CPU8080& cpu) -> uint8_t {
        cpu.regs.a = ~cpu.regs.a;
        cpu.currentInstructionTStates = 4;
        return 1;
    };

    // DAA: 0x27 - Decimal Adjust Accumulator (conforme spec Intel 8080)
    // Ajuste A après ADD/ADC/SUB/SBB pour représentation BCD décimale
    instructions[0x27] = [](CPU8080& cpu) -> uint8_t {
        uint8_t a = cpu.regs.a;
        bool cyOrig = cpu.flags.isCarry();
        bool acOrig = cpu.flags.isAuxiliaryCarry();
        
        // Étape 1 : Si nibble bas > 9 ou AC=1, ajouter 6
        if ((a & 0x0F) > 9 || acOrig) {
            a = static_cast<uint8_t>(a + 6);
        }
        
        // Étape 2 : Si nibble haut > 9 ou CY=1, ajouter 0x60
        if ((a & 0xF0) > 0x90 || cyOrig) {
            a = static_cast<uint8_t>(a + 0x60);
            cpu.flags.setCarry(true);
        }
        
        cpu.regs.a = a;
        
        // Recalculer tous les flags après DAA
        cpu.flags.setZero(a == 0);
        cpu.flags.setSign((a & 0x80) != 0);
        cpu.flags.setParity(ALU::parity(a));
        
        // AC après DAA : mis à 1 si ajustement du nibble bas a généré un carry vers bit 4
        // Selon spec Intel, AC est mis à jour selon le résultat final
        cpu.flags.setAuxiliaryCarry((a & 0x10) != 0);
        
        cpu.currentInstructionTStates = 4;
        return 1;
    };

    // INR instructions (0x04, 0x0C, 0x14, 0x1C, 0x24, 0x2C, 0x3C) — 5 T-states
    instructions[0x04] = [](CPU8080& cpu) -> uint8_t {
        cpu.regs.b = static_cast<uint8_t>(cpu.regs.b + 1);
        cpu.flags.setZero(cpu.regs.b == 0);
        cpu.flags.setSign((cpu.regs.b & 0x80) != 0);
        cpu.flags.setParity(ALU::parity(cpu.regs.b));
        cpu.flags.setAuxiliaryCarry((cpu.regs.b & 0x0F) == 0);
        return 1;
    };
    instructions[0x0C] = [](CPU8080& cpu) -> uint8_t {
        cpu.regs.c = static_cast<uint8_t>(cpu.regs.c + 1);
        cpu.flags.setZero(cpu.regs.c == 0);
        cpu.flags.setSign((cpu.regs.c & 0x80) != 0);
        cpu.flags.setParity(ALU::parity(cpu.regs.c));
        cpu.flags.setAuxiliaryCarry((cpu.regs.c & 0x0F) == 0);
        return 1;
    };
    instructions[0x14] = [](CPU8080& cpu) -> uint8_t {
        cpu.regs.d = static_cast<uint8_t>(cpu.regs.d + 1);
        cpu.flags.setZero(cpu.regs.d == 0);
        cpu.flags.setSign((cpu.regs.d & 0x80) != 0);
        cpu.flags.setParity(ALU::parity(cpu.regs.d));
        cpu.flags.setAuxiliaryCarry((cpu.regs.d & 0x0F) == 0);
        return 1;
    };
    instructions[0x1C] = [](CPU8080& cpu) -> uint8_t {
        cpu.regs.e = static_cast<uint8_t>(cpu.regs.e + 1);
        cpu.flags.setZero(cpu.regs.e == 0);
        cpu.flags.setSign((cpu.regs.e & 0x80) != 0);
        cpu.flags.setParity(ALU::parity(cpu.regs.e));
        cpu.flags.setAuxiliaryCarry((cpu.regs.e & 0x0F) == 0);
        return 1;
    };
    instructions[0x24] = [](CPU8080& cpu) -> uint8_t {
        cpu.regs.h = static_cast<uint8_t>(cpu.regs.h + 1);
        cpu.flags.setZero(cpu.regs.h == 0);
        cpu.flags.setSign((cpu.regs.h & 0x80) != 0);
        cpu.flags.setParity(ALU::parity(cpu.regs.h));
        cpu.flags.setAuxiliaryCarry((cpu.regs.h & 0x0F) == 0);
        return 1;
    };
    instructions[0x2C] = [](CPU8080& cpu) -> uint8_t {
        cpu.regs.l = static_cast<uint8_t>(cpu.regs.l + 1);
        cpu.flags.setZero(cpu.regs.l == 0);
        cpu.flags.setSign((cpu.regs.l & 0x80) != 0);
        cpu.flags.setParity(ALU::parity(cpu.regs.l));
        cpu.flags.setAuxiliaryCarry((cpu.regs.l & 0x0F) == 0);
        return 1;
    };
    instructions[0x3C] = [](CPU8080& cpu) -> uint8_t {
        cpu.regs.a = static_cast<uint8_t>(cpu.regs.a + 1);
        cpu.flags.setZero(cpu.regs.a == 0);
        cpu.flags.setSign((cpu.regs.a & 0x80) != 0);
        cpu.flags.setParity(ALU::parity(cpu.regs.a));
        cpu.flags.setAuxiliaryCarry((cpu.regs.a & 0x0F) == 0);
        return 1;
    };

    // INR M: 0x34 — 10 T-states
    instructions[0x34] = [](CPU8080& cpu) -> uint8_t {
        uint16_t hl = (cpu.regs.h << 8) | cpu.regs.l;
        uint8_t val = static_cast<uint8_t>(cpu.memory.read(hl) + 1);
        cpu.memory.write(hl, val);
        cpu.flags.setZero(val == 0);
        cpu.flags.setSign((val & 0x80) != 0);
        cpu.flags.setParity(ALU::parity(val));
        cpu.flags.setAuxiliaryCarry((val & 0x0F) == 0);
        return 1;
    };

    // DCR instructions (0x05, 0x0D, 0x15, 0x1D, 0x25, 0x2D, 0x3D) — 5 T-states
    instructions[0x05] = [](CPU8080& cpu) -> uint8_t {
        cpu.regs.b = static_cast<uint8_t>(cpu.regs.b - 1);
        cpu.flags.setZero(cpu.regs.b == 0);
        cpu.flags.setSign((cpu.regs.b & 0x80) != 0);
        cpu.flags.setParity(ALU::parity(cpu.regs.b));
        cpu.flags.setAuxiliaryCarry((cpu.regs.b & 0x0F) == 0x0F);
        return 1;
    };
    instructions[0x0D] = [](CPU8080& cpu) -> uint8_t {
        cpu.regs.c = static_cast<uint8_t>(cpu.regs.c - 1);
        cpu.flags.setZero(cpu.regs.c == 0);
        cpu.flags.setSign((cpu.regs.c & 0x80) != 0);
        cpu.flags.setParity(ALU::parity(cpu.regs.c));
        cpu.flags.setAuxiliaryCarry((cpu.regs.c & 0x0F) == 0x0F);
        return 1;
    };
    instructions[0x15] = [](CPU8080& cpu) -> uint8_t {
        cpu.regs.d = static_cast<uint8_t>(cpu.regs.d - 1);
        cpu.flags.setZero(cpu.regs.d == 0);
        cpu.flags.setSign((cpu.regs.d & 0x80) != 0);
        cpu.flags.setParity(ALU::parity(cpu.regs.d));
        cpu.flags.setAuxiliaryCarry((cpu.regs.d & 0x0F) == 0x0F);
        return 1;
    };
    instructions[0x1D] = [](CPU8080& cpu) -> uint8_t {
        cpu.regs.e = static_cast<uint8_t>(cpu.regs.e - 1);
        cpu.flags.setZero(cpu.regs.e == 0);
        cpu.flags.setSign((cpu.regs.e & 0x80) != 0);
        cpu.flags.setParity(ALU::parity(cpu.regs.e));
        cpu.flags.setAuxiliaryCarry((cpu.regs.e & 0x0F) == 0x0F);
        return 1;
    };
    instructions[0x25] = [](CPU8080& cpu) -> uint8_t {
        cpu.regs.h = static_cast<uint8_t>(cpu.regs.h - 1);
        cpu.flags.setZero(cpu.regs.h == 0);
        cpu.flags.setSign((cpu.regs.h & 0x80) != 0);
        cpu.flags.setParity(ALU::parity(cpu.regs.h));
        cpu.flags.setAuxiliaryCarry((cpu.regs.h & 0x0F) == 0x0F);
        return 1;
    };
    instructions[0x2D] = [](CPU8080& cpu) -> uint8_t {
        cpu.regs.l = static_cast<uint8_t>(cpu.regs.l - 1);
        cpu.flags.setZero(cpu.regs.l == 0);
        cpu.flags.setSign((cpu.regs.l & 0x80) != 0);
        cpu.flags.setParity(ALU::parity(cpu.regs.l));
        cpu.flags.setAuxiliaryCarry((cpu.regs.l & 0x0F) == 0x0F);
        return 1;
    };
    instructions[0x3D] = [](CPU8080& cpu) -> uint8_t {
        cpu.regs.a = static_cast<uint8_t>(cpu.regs.a - 1);
        cpu.flags.setZero(cpu.regs.a == 0);
        cpu.flags.setSign((cpu.regs.a & 0x80) != 0);
        cpu.flags.setParity(ALU::parity(cpu.regs.a));
        cpu.flags.setAuxiliaryCarry((cpu.regs.a & 0x0F) == 0x0F);
        return 1;
    };

    // DCR M: 0x35 — 10 T-states
    instructions[0x35] = [](CPU8080& cpu) -> uint8_t {
        uint16_t hl = (cpu.regs.h << 8) | cpu.regs.l;
        uint8_t val = static_cast<uint8_t>(cpu.memory.read(hl) - 1);
        cpu.memory.write(hl, val);
        cpu.flags.setZero(val == 0);
        cpu.flags.setSign((val & 0x80) != 0);
        cpu.flags.setParity(ALU::parity(val));
        cpu.flags.setAuxiliaryCarry((val & 0x0F) == 0x0F);
        return 1;
    };

    // HLT: 0x76 — 7 T-states
    instructions[0x76] = [](CPU8080& cpu) -> uint8_t {
        cpu.halted = true;
        return 1;
    };

    // ==================== DATA TRANSFER INSTRUCTIONS ====================
    // MOV dst, src: 0x40 + (dst << 3) + src (except MOV M,M is invalid)
    for(int dst = 0; dst < 8; ++dst) {
        for(int src = 0; src < 8; ++src) {
            if(dst == 6 && src == 6) continue; // MOV M, M is invalid (opcode 0x7F = HLT)
            uint8_t opcode = 0x40 + (dst << 3) + src;
            bool involvesMemory = (dst == 6 || src == 6);
            
            instructions[opcode] = [dst, src, involvesMemory](CPU8080& cpu) -> uint8_t {
                uint8_t val;
                if(src == 6) {
                    val = cpu.memory.read((cpu.regs.h << 8) | cpu.regs.l);
                } else {
                    switch(src) {
                        case 0: val = cpu.regs.b; break;
                        case 1: val = cpu.regs.c; break;
                        case 2: val = cpu.regs.d; break;
                        case 3: val = cpu.regs.e; break;
                        case 4: val = cpu.regs.h; break;
                        case 5: val = cpu.regs.l; break;
                        case 7: val = cpu.regs.a; break;
                        default: val = 0;
                    }
                }
                switch(dst) {
                    case 0: cpu.regs.b = val; break;
                    case 1: cpu.regs.c = val; break;
                    case 2: cpu.regs.d = val; break;
                    case 3: cpu.regs.e = val; break;
                    case 4: cpu.regs.h = val; break;
                    case 5: cpu.regs.l = val; break;
                    case 6: cpu.memory.write((cpu.regs.h << 8) | cpu.regs.l, val); break;
                    case 7: cpu.regs.a = val; break;
                }
                return 1;
            };
        }
    }

    // STAX B/D and LDAX B/D — 7 T-states (memory access)
    instructions[0x02] = [](CPU8080& cpu) -> uint8_t { 
        cpu.memory.write((cpu.regs.b << 8) | cpu.regs.c, cpu.regs.a); 
        return 1; 
    };
    instructions[0x12] = [](CPU8080& cpu) -> uint8_t { 
        cpu.memory.write((cpu.regs.d << 8) | cpu.regs.e, cpu.regs.a); 
        return 1; 
    };
    instructions[0x0A] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.a = cpu.memory.read((cpu.regs.b << 8) | cpu.regs.c); 
        return 1; 
    };
    instructions[0x1A] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.a = cpu.memory.read((cpu.regs.d << 8) | cpu.regs.e); 
        return 1; 
    };

    // ==================== ALU INSTRUCTIONS (on register/memory) ====================
    // ADD B-C, D-E, H-L, M, A — 0x80-0x87
    instructions[0x80] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::add(cpu.regs.a, cpu.regs.b, cpu.flags); return 1; };
    instructions[0x81] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::add(cpu.regs.a, cpu.regs.c, cpu.flags); return 1; };
    instructions[0x82] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::add(cpu.regs.a, cpu.regs.d, cpu.flags); return 1; };
    instructions[0x83] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::add(cpu.regs.a, cpu.regs.e, cpu.flags); return 1; };
    instructions[0x84] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::add(cpu.regs.a, cpu.regs.h, cpu.flags); return 1; };
    instructions[0x85] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::add(cpu.regs.a, cpu.regs.l, cpu.flags); return 1; };
    instructions[0x86] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.a = ALU::add(cpu.regs.a, cpu.memory.read((cpu.regs.h << 8) | cpu.regs.l), cpu.flags); 
        return 1; 
    };
    instructions[0x87] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::add(cpu.regs.a, cpu.regs.a, cpu.flags); return 1; };

    // ADC B-C, D-E, H-L, M, A — 0x88-0x8F
    instructions[0x88] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::adc(cpu.regs.a, cpu.regs.b, cpu.flags); return 1; };
    instructions[0x89] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::adc(cpu.regs.a, cpu.regs.c, cpu.flags); return 1; };
    instructions[0x8A] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::adc(cpu.regs.a, cpu.regs.d, cpu.flags); return 1; };
    instructions[0x8B] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::adc(cpu.regs.a, cpu.regs.e, cpu.flags); return 1; };
    instructions[0x8C] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::adc(cpu.regs.a, cpu.regs.h, cpu.flags); return 1; };
    instructions[0x8D] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::adc(cpu.regs.a, cpu.regs.l, cpu.flags); return 1; };
    instructions[0x8E] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.a = ALU::adc(cpu.regs.a, cpu.memory.read((cpu.regs.h << 8) | cpu.regs.l), cpu.flags); 
        return 1; 
    };
    instructions[0x8F] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::adc(cpu.regs.a, cpu.regs.a, cpu.flags); return 1; };

    // SUB B-C, D-E, H-L, M, A — 0x90-0x97
    instructions[0x90] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::sub(cpu.regs.a, cpu.regs.b, cpu.flags); return 1; };
    instructions[0x91] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::sub(cpu.regs.a, cpu.regs.c, cpu.flags); return 1; };
    instructions[0x92] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::sub(cpu.regs.a, cpu.regs.d, cpu.flags); return 1; };
    instructions[0x93] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::sub(cpu.regs.a, cpu.regs.e, cpu.flags); return 1; };
    instructions[0x94] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::sub(cpu.regs.a, cpu.regs.h, cpu.flags); return 1; };
    instructions[0x95] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::sub(cpu.regs.a, cpu.regs.l, cpu.flags); return 1; };
    instructions[0x96] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.a = ALU::sub(cpu.regs.a, cpu.memory.read((cpu.regs.h << 8) | cpu.regs.l), cpu.flags); 
        return 1; 
    };
    instructions[0x97] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::sub(cpu.regs.a, cpu.regs.a, cpu.flags); return 1; };

    // SBB B-C, D-E, H-L, M, A — 0x98-0x9F
    instructions[0x98] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::sbb(cpu.regs.a, cpu.regs.b, cpu.flags); return 1; };
    instructions[0x99] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::sbb(cpu.regs.a, cpu.regs.c, cpu.flags); return 1; };
    instructions[0x9A] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::sbb(cpu.regs.a, cpu.regs.d, cpu.flags); return 1; };
    instructions[0x9B] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::sbb(cpu.regs.a, cpu.regs.e, cpu.flags); return 1; };
    instructions[0x9C] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::sbb(cpu.regs.a, cpu.regs.h, cpu.flags); return 1; };
    instructions[0x9D] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::sbb(cpu.regs.a, cpu.regs.l, cpu.flags); return 1; };
    instructions[0x9E] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.a = ALU::sbb(cpu.regs.a, cpu.memory.read((cpu.regs.h << 8) | cpu.regs.l), cpu.flags); 
        return 1; 
    };
    instructions[0x9F] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::sbb(cpu.regs.a, cpu.regs.a, cpu.flags); return 1; };

    // ANA B-C, D-E, H-L, M, A — 0xA0-0xA7
    instructions[0xA0] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::and_op(cpu.regs.a, cpu.regs.b, cpu.flags); return 1; };
    instructions[0xA1] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::and_op(cpu.regs.a, cpu.regs.c, cpu.flags); return 1; };
    instructions[0xA2] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::and_op(cpu.regs.a, cpu.regs.d, cpu.flags); return 1; };
    instructions[0xA3] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::and_op(cpu.regs.a, cpu.regs.e, cpu.flags); return 1; };
    instructions[0xA4] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::and_op(cpu.regs.a, cpu.regs.h, cpu.flags); return 1; };
    instructions[0xA5] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::and_op(cpu.regs.a, cpu.regs.l, cpu.flags); return 1; };
    instructions[0xA6] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.a = ALU::and_op(cpu.regs.a, cpu.memory.read((cpu.regs.h << 8) | cpu.regs.l), cpu.flags); 
        return 1; 
    };
    instructions[0xA7] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::and_op(cpu.regs.a, cpu.regs.a, cpu.flags); return 1; };

    // XRA B-C, D-E, H-L, M, A — 0xA8-0xAF
    instructions[0xA8] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::xor_op(cpu.regs.a, cpu.regs.b, cpu.flags); return 1; };
    instructions[0xA9] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::xor_op(cpu.regs.a, cpu.regs.c, cpu.flags); return 1; };
    instructions[0xAA] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::xor_op(cpu.regs.a, cpu.regs.d, cpu.flags); return 1; };
    instructions[0xAB] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::xor_op(cpu.regs.a, cpu.regs.e, cpu.flags); return 1; };
    instructions[0xAC] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::xor_op(cpu.regs.a, cpu.regs.h, cpu.flags); return 1; };
    instructions[0xAD] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::xor_op(cpu.regs.a, cpu.regs.l, cpu.flags); return 1; };
    instructions[0xAE] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.a = ALU::xor_op(cpu.regs.a, cpu.memory.read((cpu.regs.h << 8) | cpu.regs.l), cpu.flags); 
        return 1; 
    };
    instructions[0xAF] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::xor_op(cpu.regs.a, cpu.regs.a, cpu.flags); return 1; };

    // ORA B-C, D-E, H-L, M, A — 0xB0-0xB7
    instructions[0xB0] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::or_op(cpu.regs.a, cpu.regs.b, cpu.flags); return 1; };
    instructions[0xB1] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::or_op(cpu.regs.a, cpu.regs.c, cpu.flags); return 1; };
    instructions[0xB2] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::or_op(cpu.regs.a, cpu.regs.d, cpu.flags); return 1; };
    instructions[0xB3] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::or_op(cpu.regs.a, cpu.regs.e, cpu.flags); return 1; };
    instructions[0xB4] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::or_op(cpu.regs.a, cpu.regs.h, cpu.flags); return 1; };
    instructions[0xB5] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::or_op(cpu.regs.a, cpu.regs.l, cpu.flags); return 1; };
    instructions[0xB6] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.a = ALU::or_op(cpu.regs.a, cpu.memory.read((cpu.regs.h << 8) | cpu.regs.l), cpu.flags); 
        return 1; 
    };
    instructions[0xB7] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::or_op(cpu.regs.a, cpu.regs.a, cpu.flags); return 1; };

    // CMP B-C, D-E, H-L, M, A — 0xB8-0xBF
    instructions[0xB8] = [](CPU8080& cpu) -> uint8_t { ALU::cmp(cpu.regs.a, cpu.regs.b, cpu.flags); return 1; };
    instructions[0xB9] = [](CPU8080& cpu) -> uint8_t { ALU::cmp(cpu.regs.a, cpu.regs.c, cpu.flags); return 1; };
    instructions[0xBA] = [](CPU8080& cpu) -> uint8_t { ALU::cmp(cpu.regs.a, cpu.regs.d, cpu.flags); return 1; };
    instructions[0xBB] = [](CPU8080& cpu) -> uint8_t { ALU::cmp(cpu.regs.a, cpu.regs.e, cpu.flags); return 1; };
    instructions[0xBC] = [](CPU8080& cpu) -> uint8_t { ALU::cmp(cpu.regs.a, cpu.regs.h, cpu.flags); return 1; };
    instructions[0xBD] = [](CPU8080& cpu) -> uint8_t { ALU::cmp(cpu.regs.a, cpu.regs.l, cpu.flags); return 1; };
    instructions[0xBE] = [](CPU8080& cpu) -> uint8_t { 
        ALU::cmp(cpu.regs.a, cpu.memory.read((cpu.regs.h << 8) | cpu.regs.l), cpu.flags); 
        return 1; 
    };
    instructions[0xBF] = [](CPU8080& cpu) -> uint8_t { ALU::cmp(cpu.regs.a, cpu.regs.a, cpu.flags); return 1; };

    // ==================== ROTATION INSTRUCTIONS ====================
    instructions[0x07] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::rlc(cpu.regs.a, cpu.flags); return 1; };
    instructions[0x0F] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::rrc(cpu.regs.a, cpu.flags); return 1; };
    instructions[0x17] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::ral(cpu.regs.a, cpu.flags); return 1; };
    instructions[0x1F] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = ALU::rar(cpu.regs.a, cpu.flags); return 1; };

    // ==================== IMMEDIATE INSTRUCTIONS ====================
    // MVI reg, imm: 0x06+reg*2 for B,C,D,E,H,L,A and 0x36 for M — 7 T-states
    instructions[0x06] = [](CPU8080& cpu) -> uint8_t { cpu.regs.b = cpu.memory.read(cpu.pc + 1); return 2; };
    instructions[0x0E] = [](CPU8080& cpu) -> uint8_t { cpu.regs.c = cpu.memory.read(cpu.pc + 1); return 2; };
    instructions[0x16] = [](CPU8080& cpu) -> uint8_t { cpu.regs.d = cpu.memory.read(cpu.pc + 1); return 2; };
    instructions[0x1E] = [](CPU8080& cpu) -> uint8_t { cpu.regs.e = cpu.memory.read(cpu.pc + 1); return 2; };
    instructions[0x26] = [](CPU8080& cpu) -> uint8_t { cpu.regs.h = cpu.memory.read(cpu.pc + 1); return 2; };
    instructions[0x2E] = [](CPU8080& cpu) -> uint8_t { cpu.regs.l = cpu.memory.read(cpu.pc + 1); return 2; };
    instructions[0x36] = [](CPU8080& cpu) -> uint8_t { 
        cpu.memory.write((cpu.regs.h << 8) | cpu.regs.l, cpu.memory.read(cpu.pc + 1)); 
        return 2; 
    };
    instructions[0x3E] = [](CPU8080& cpu) -> uint8_t { cpu.regs.a = cpu.memory.read(cpu.pc + 1); return 2; };

    // ADI/ACI/SUI/SBI/ANI/XRI/ORI/CPI — 7 T-states
    instructions[0xC6] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.a = ALU::add(cpu.regs.a, cpu.memory.read(cpu.pc + 1), cpu.flags); return 2; 
    };
    instructions[0xCE] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.a = ALU::adc(cpu.regs.a, cpu.memory.read(cpu.pc + 1), cpu.flags); return 2; 
    };
    instructions[0xD6] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.a = ALU::sub(cpu.regs.a, cpu.memory.read(cpu.pc + 1), cpu.flags); return 2; 
    };
    instructions[0xDE] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.a = ALU::sbb(cpu.regs.a, cpu.memory.read(cpu.pc + 1), cpu.flags); return 2; 
    };
    instructions[0xE6] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.a = ALU::and_op(cpu.regs.a, cpu.memory.read(cpu.pc + 1), cpu.flags); return 2; 
    };
    instructions[0xEE] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.a = ALU::xor_op(cpu.regs.a, cpu.memory.read(cpu.pc + 1), cpu.flags); return 2; 
    };
    instructions[0xF6] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.a = ALU::or_op(cpu.regs.a, cpu.memory.read(cpu.pc + 1), cpu.flags); return 2; 
    };
    instructions[0xFE] = [](CPU8080& cpu) -> uint8_t { 
        ALU::cmp(cpu.regs.a, cpu.memory.read(cpu.pc + 1), cpu.flags); return 2; 
    };

    // ==================== REGISTER PAIR INSTRUCTIONS ====================
    // LXI rp, imm: 0x01(B), 0x11(D), 0x21(H), 0x31(SP) — 10 T-states
    instructions[0x01] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.b = cpu.memory.read(cpu.pc + 2); cpu.regs.c = cpu.memory.read(cpu.pc + 1); return 3; 
    };
    instructions[0x11] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.d = cpu.memory.read(cpu.pc + 2); cpu.regs.e = cpu.memory.read(cpu.pc + 1); return 3; 
    };
    instructions[0x21] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.h = cpu.memory.read(cpu.pc + 2); cpu.regs.l = cpu.memory.read(cpu.pc + 1); return 3; 
    };
    instructions[0x31] = [](CPU8080& cpu) -> uint8_t { 
        cpu.sp = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1); return 3; 
    };

    // DAD rp: 0x09(B), 0x19(D), 0x29(H), 0x39(SP) — 10 T-states
    instructions[0x09] = [](CPU8080& cpu) -> uint8_t {
        uint16_t hl = (cpu.regs.h << 8) | cpu.regs.l;
        uint16_t bc = (cpu.regs.b << 8) | cpu.regs.c;
        uint16_t res = hl + bc;
        cpu.regs.h = static_cast<uint8_t>(res >> 8);
        cpu.regs.l = static_cast<uint8_t>(res & 0xFF);
        return 1;
    };
    instructions[0x19] = [](CPU8080& cpu) -> uint8_t {
        uint16_t hl = (cpu.regs.h << 8) | cpu.regs.l;
        uint16_t de = (cpu.regs.d << 8) | cpu.regs.e;
        uint16_t res = hl + de;
        cpu.regs.h = static_cast<uint8_t>(res >> 8);
        cpu.regs.l = static_cast<uint8_t>(res & 0xFF);
        return 1;
    };
    instructions[0x29] = [](CPU8080& cpu) -> uint8_t {
        uint16_t hl = (cpu.regs.h << 8) | cpu.regs.l;
        uint16_t res = hl + hl;
        cpu.regs.h = static_cast<uint8_t>(res >> 8);
        cpu.regs.l = static_cast<uint8_t>(res & 0xFF);
        return 1;
    };
    instructions[0x39] = [](CPU8080& cpu) -> uint8_t {
        uint16_t hl = (cpu.regs.h << 8) | cpu.regs.l;
        uint16_t res = hl + cpu.sp;
        cpu.regs.h = static_cast<uint8_t>(res >> 8);
        cpu.regs.l = static_cast<uint8_t>(res & 0xFF);
        return 1;
    };

    // INX rp: 0x03(B), 0x13(D), 0x23(H), 0x33(SP) — 5 T-states
    instructions[0x03] = [](CPU8080& cpu) -> uint8_t { 
        uint16_t bc = (cpu.regs.b << 8) | cpu.regs.c; ++bc; 
        cpu.regs.b = static_cast<uint8_t>(bc >> 8); cpu.regs.c = static_cast<uint8_t>(bc & 0xFF); return 1; 
    };
    instructions[0x13] = [](CPU8080& cpu) -> uint8_t { 
        uint16_t de = (cpu.regs.d << 8) | cpu.regs.e; ++de; 
        cpu.regs.d = static_cast<uint8_t>(de >> 8); cpu.regs.e = static_cast<uint8_t>(de & 0xFF); return 1; 
    };
    instructions[0x23] = [](CPU8080& cpu) -> uint8_t { 
        uint16_t hl = (cpu.regs.h << 8) | cpu.regs.l; ++hl; 
        cpu.regs.h = static_cast<uint8_t>(hl >> 8); cpu.regs.l = static_cast<uint8_t>(hl & 0xFF); return 1; 
    };
    instructions[0x33] = [](CPU8080& cpu) -> uint8_t { ++cpu.sp; return 1; };

    // DCX rp: 0x0B(B), 0x1B(D), 0x2B(H), 0x3B(SP) — 5 T-states
    instructions[0x0B] = [](CPU8080& cpu) -> uint8_t { 
        uint16_t bc = (cpu.regs.b << 8) | cpu.regs.c; --bc; 
        cpu.regs.b = static_cast<uint8_t>(bc >> 8); cpu.regs.c = static_cast<uint8_t>(bc & 0xFF); return 1; 
    };
    instructions[0x1B] = [](CPU8080& cpu) -> uint8_t { 
        uint16_t de = (cpu.regs.d << 8) | cpu.regs.e; --de; 
        cpu.regs.d = static_cast<uint8_t>(de >> 8); cpu.regs.e = static_cast<uint8_t>(de & 0xFF); return 1; 
    };
    instructions[0x2B] = [](CPU8080& cpu) -> uint8_t { 
        uint16_t hl = (cpu.regs.h << 8) | cpu.regs.l; --hl; 
        cpu.regs.h = static_cast<uint8_t>(hl >> 8); cpu.regs.l = static_cast<uint8_t>(hl & 0xFF); return 1; 
    };
    instructions[0x3B] = [](CPU8080& cpu) -> uint8_t { --cpu.sp; return 1; };

    // XCHG: 0xEB, XTHL: 0xE3, SPHL: 0xF9, PCHL: 0xE9
    instructions[0xEB] = [](CPU8080& cpu) -> uint8_t { 
        std::swap(cpu.regs.h, cpu.regs.d); std::swap(cpu.regs.l, cpu.regs.e); return 1; 
    };
    instructions[0xE3] = [](CPU8080& cpu) -> uint8_t {
        uint8_t tmpL = cpu.memory.read(cpu.sp);
        uint8_t tmpH = cpu.memory.read(cpu.sp + 1);
        cpu.memory.write(cpu.sp, cpu.regs.l);
        cpu.memory.write(cpu.sp + 1, cpu.regs.h);
        cpu.regs.l = tmpL;
        cpu.regs.h = tmpH;
        return 1;
    };
    instructions[0xF9] = [](CPU8080& cpu) -> uint8_t { 
        cpu.sp = (cpu.regs.h << 8) | cpu.regs.l; return 1; 
    };
    // PCHL: 0xE9 — return 0 so step() doesn't increment pc after setting it to HL
    instructions[0xE9] = [](CPU8080& cpu) -> uint8_t { 
        cpu.pc = (cpu.regs.h << 8) | cpu.regs.l; return 0; 
    };

    // ==================== PUSH / POP INSTRUCTIONS — BUG #4 FIX: Stack ordre correct ====================
    // PUSH B/D/H/PSW: 0xC5, 0xD5, 0xE5, 0xF5 — 11 T-states
    // PUSH : SP -= 2 AVANT d'écrire (conforme spec Intel 8080)
    instructions[0xC5] = [](CPU8080& cpu) -> uint8_t { 
        cpu.sp -= 2; cpu.memory.write(cpu.sp + 1, cpu.regs.b); cpu.memory.write(cpu.sp, cpu.regs.c); return 1; 
    };
    instructions[0xD5] = [](CPU8080& cpu) -> uint8_t { 
        cpu.sp -= 2; cpu.memory.write(cpu.sp + 1, cpu.regs.d); cpu.memory.write(cpu.sp, cpu.regs.e); return 1; 
    };
    instructions[0xE5] = [](CPU8080& cpu) -> uint8_t { 
        cpu.sp -= 2; cpu.memory.write(cpu.sp + 1, cpu.regs.h); cpu.memory.write(cpu.sp, cpu.regs.l); return 1; 
    };
    instructions[0xF5] = [](CPU8080& cpu) -> uint8_t { 
        cpu.sp -= 2; cpu.memory.write(cpu.sp + 1, cpu.regs.a); cpu.memory.write(cpu.sp, cpu.flags.getByte()); return 1; 
    };

    // POP rp: 0xC1(B), 0xD1(D), 0xE1(H), 0xF1(PSW) — 10 T-states
    // POP : lire puis SP += 2 (conforme spec Intel 8080)
    instructions[0xC1] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.c = cpu.memory.read(cpu.sp); cpu.regs.b = cpu.memory.read(cpu.sp + 1); cpu.sp += 2; return 1; 
    };
    instructions[0xD1] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.e = cpu.memory.read(cpu.sp); cpu.regs.d = cpu.memory.read(cpu.sp + 1); cpu.sp += 2; return 1; 
    };
    instructions[0xE1] = [](CPU8080& cpu) -> uint8_t { 
        cpu.regs.l = cpu.memory.read(cpu.sp); cpu.regs.h = cpu.memory.read(cpu.sp + 1); cpu.sp += 2; return 1; 
    };
    instructions[0xF1] = [](CPU8080& cpu) -> uint8_t { 
        cpu.flags.setByte(cpu.memory.read(cpu.sp)); cpu.regs.a = cpu.memory.read(cpu.sp + 1); cpu.sp += 2; return 1; 
    };

    // ==================== DIRECT ADDRESS INSTRUCTIONS ====================
    instructions[0x32] = [](CPU8080& cpu) -> uint8_t { 
        uint16_t addr = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1); 
        cpu.memory.write(addr, cpu.regs.a); return 3; 
    };
    instructions[0x3A] = [](CPU8080& cpu) -> uint8_t { 
        uint16_t addr = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1); 
        cpu.regs.a = cpu.memory.read(addr); return 3; 
    };
    instructions[0x22] = [](CPU8080& cpu) -> uint8_t { 
        uint16_t addr = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1); 
        cpu.memory.write(addr, cpu.regs.l); cpu.memory.write(addr + 1, cpu.regs.h); return 3; 
    };
    instructions[0x2A] = [](CPU8080& cpu) -> uint8_t { 
        uint16_t addr = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1); 
        cpu.regs.l = cpu.memory.read(addr); cpu.regs.h = cpu.memory.read(addr + 1); return 3; 
    };

    // ==================== JUMP INSTRUCTIONS — BUG #4 FIX: T-states (10), return 0 when taken ====================
    instructions[0xC3] = [](CPU8080& cpu) -> uint8_t {
        cpu.pc = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
        return 0;
    };

    // JC: 0xDA — pris: 10, non-pris: 10
    instructions[0xDA] = [](CPU8080& cpu) -> uint8_t {
        if(cpu.flags.isCarry()) {
            cpu.pc = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
            return 0;
        }
        return 3;
    };

    // JNC: 0xD2 — pris: 10, non-pris: 10
    instructions[0xD2] = [](CPU8080& cpu) -> uint8_t {
        if(!cpu.flags.isCarry()) {
            cpu.pc = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
            return 0;
        }
        return 3;
    };

    // JZ: 0xCA — pris: 10, non-pris: 10
    instructions[0xCA] = [](CPU8080& cpu) -> uint8_t {
        if(cpu.flags.isZero()) {
            cpu.pc = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
            return 0;
        }
        return 3;
    };

    // JNZ: 0xC2 — pris: 10, non-pris: 10
    instructions[0xC2] = [](CPU8080& cpu) -> uint8_t {
        if(!cpu.flags.isZero()) {
            cpu.pc = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
            return 0;
        }
        return 3;
    };

    // JM: 0xFA — pris: 10, non-pris: 10
    instructions[0xFA] = [](CPU8080& cpu) -> uint8_t {
        if(cpu.flags.isSign()) {
            cpu.pc = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
            return 0;
        }
        return 3;
    };

    // JP: 0xF2 — pris: 10, non-pris: 10
    instructions[0xF2] = [](CPU8080& cpu) -> uint8_t {
        if(!cpu.flags.isSign()) {
            cpu.pc = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
            return 0;
        }
        return 3;
    };

    // JPE: 0xEA — pris: 10, non-pris: 10
    instructions[0xEA] = [](CPU8080& cpu) -> uint8_t {
        if(cpu.flags.isParity()) {
            cpu.pc = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
            return 0;
        }
        return 3;
    };

    // JPO: 0xE2 — pris: 10, non-pris: 10
    instructions[0xE2] = [](CPU8080& cpu) -> uint8_t {
        if(!cpu.flags.isParity()) {
            cpu.pc = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
            return 0;
        }
        return 3;
    };

    // ==================== CALL INSTRUCTIONS — BUG #4 FIX: T-states (17), push pc+3, return 0 ====================
    instructions[0xCD] = [](CPU8080& cpu) -> uint8_t {
        uint16_t addr = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
        uint16_t retAddr = cpu.pc + 3;
        cpu.sp -= 2;
        cpu.memory.write(cpu.sp + 1, static_cast<uint8_t>(retAddr >> 8));
        cpu.memory.write(cpu.sp, static_cast<uint8_t>(retAddr & 0xFF));
        cpu.pc = addr;
        return 0;
    };

    // CC (CY=1): 0xDC — pris: 17, non-pris: 11
    instructions[0xDC] = [](CPU8080& cpu) -> uint8_t {
        uint16_t addr = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
        if(cpu.flags.isCarry()) {
            uint16_t retAddr = cpu.pc + 3;
            cpu.sp -= 2;
            cpu.memory.write(cpu.sp + 1, static_cast<uint8_t>(retAddr >> 8));
            cpu.memory.write(cpu.sp, static_cast<uint8_t>(retAddr & 0xFF));
            cpu.pc = addr;
            return 0;
        }
        return 3;
    };

    // CNC (CY=0): 0xD4 — pris: 17, non-pris: 11
    instructions[0xD4] = [](CPU8080& cpu) -> uint8_t {
        uint16_t addr = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
        if(!cpu.flags.isCarry()) {
            uint16_t retAddr = cpu.pc + 3;
            cpu.sp -= 2;
            cpu.memory.write(cpu.sp + 1, static_cast<uint8_t>(retAddr >> 8));
            cpu.memory.write(cpu.sp, static_cast<uint8_t>(retAddr & 0xFF));
            cpu.pc = addr;
            return 0;
        }
        return 3;
    };

    // CZ (Z=1): 0xCC — pris: 17, non-pris: 11
    instructions[0xCC] = [](CPU8080& cpu) -> uint8_t {
        uint16_t addr = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
        if(cpu.flags.isZero()) {
            uint16_t retAddr = cpu.pc + 3;
            cpu.sp -= 2;
            cpu.memory.write(cpu.sp + 1, static_cast<uint8_t>(retAddr >> 8));
            cpu.memory.write(cpu.sp, static_cast<uint8_t>(retAddr & 0xFF));
            cpu.pc = addr;
            return 0;
        }
        return 3;
    };

    // CNZ (Z=0): 0xC4 — pris: 17, non-pris: 11
    instructions[0xC4] = [](CPU8080& cpu) -> uint8_t {
        uint16_t addr = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
        if(!cpu.flags.isZero()) {
            uint16_t retAddr = cpu.pc + 3;
            cpu.sp -= 2;
            cpu.memory.write(cpu.sp + 1, static_cast<uint8_t>(retAddr >> 8));
            cpu.memory.write(cpu.sp, static_cast<uint8_t>(retAddr & 0xFF));
            cpu.pc = addr;
            return 0;
        }
        return 3;
    };

    // CM (S=1): 0xFC — pris: 17, non-pris: 11
    instructions[0xFC] = [](CPU8080& cpu) -> uint8_t {
        uint16_t addr = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
        if(cpu.flags.isSign()) {
            uint16_t retAddr = cpu.pc + 3;
            cpu.sp -= 2;
            cpu.memory.write(cpu.sp + 1, static_cast<uint8_t>(retAddr >> 8));
            cpu.memory.write(cpu.sp, static_cast<uint8_t>(retAddr & 0xFF));
            cpu.pc = addr;
            return 0;
        }
        return 3;
    };

    // CP (S=0): 0xF4 — pris: 17, non-pris: 11
    instructions[0xF4] = [](CPU8080& cpu) -> uint8_t {
        uint16_t addr = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
        if(!cpu.flags.isSign()) {
            uint16_t retAddr = cpu.pc + 3;
            cpu.sp -= 2;
            cpu.memory.write(cpu.sp + 1, static_cast<uint8_t>(retAddr >> 8));
            cpu.memory.write(cpu.sp, static_cast<uint8_t>(retAddr & 0xFF));
            cpu.pc = addr;
            return 0;
        }
        return 3;
    };

    // CPE (P=1): 0xEC — pris: 17, non-pris: 11
    instructions[0xEC] = [](CPU8080& cpu) -> uint8_t {
        uint16_t addr = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
        if(cpu.flags.isParity()) {
            uint16_t retAddr = cpu.pc + 3;
            cpu.sp -= 2;
            cpu.memory.write(cpu.sp + 1, static_cast<uint8_t>(retAddr >> 8));
            cpu.memory.write(cpu.sp, static_cast<uint8_t>(retAddr & 0xFF));
            cpu.pc = addr;
            return 0;
        }
        return 3;
    };

    // CPO (P=0): 0xE4 — pris: 17, non-pris: 11
    instructions[0xE4] = [](CPU8080& cpu) -> uint8_t {
        uint16_t addr = (cpu.memory.read(cpu.pc + 2) << 8) | cpu.memory.read(cpu.pc + 1);
        if(!cpu.flags.isParity()) {
            uint16_t retAddr = cpu.pc + 3;
            cpu.sp -= 2;
            cpu.memory.write(cpu.sp + 1, static_cast<uint8_t>(retAddr >> 8));
            cpu.memory.write(cpu.sp, static_cast<uint8_t>(retAddr & 0xFF));
            cpu.pc = addr;
            return 0;
        }
        return 3;
    };

    // ==================== RETURN INSTRUCTIONS — BUG #4 FIX: T-states (10/11/5), return 0 when taken ====================
    instructions[0xC9] = [](CPU8080& cpu) -> uint8_t {
        cpu.pc = cpu.memory.read(cpu.sp);
        cpu.pc |= (cpu.memory.read(cpu.sp + 1) << 8);
        cpu.sp += 2;
        return 0;
    };

    // RC: 0xD8 — pris: 11, non-pris: 5
    instructions[0xD8] = [](CPU8080& cpu) -> uint8_t {
        if(cpu.flags.isCarry()) {
            cpu.pc = cpu.memory.read(cpu.sp);
            cpu.pc |= (cpu.memory.read(cpu.sp + 1) << 8);
            cpu.sp += 2;
            return 0;
        }
        return 1;
    };

    // RNC: 0xD0 — pris: 11, non-pris: 5
    instructions[0xD0] = [](CPU8080& cpu) -> uint8_t {
        if(!cpu.flags.isCarry()) {
            cpu.pc = cpu.memory.read(cpu.sp);
            cpu.pc |= (cpu.memory.read(cpu.sp + 1) << 8);
            cpu.sp += 2;
            return 0;
        }
        return 1;
    };

    // RZ: 0xC8 — pris: 11, non-pris: 5
    instructions[0xC8] = [](CPU8080& cpu) -> uint8_t {
        if(cpu.flags.isZero()) {
            cpu.pc = cpu.memory.read(cpu.sp);
            cpu.pc |= (cpu.memory.read(cpu.sp + 1) << 8);
            cpu.sp += 2;
            return 0;
        }
        return 1;
    };

    // RNZ: 0xC0 — pris: 11, non-pris: 5
    instructions[0xC0] = [](CPU8080& cpu) -> uint8_t {
        if(!cpu.flags.isZero()) {
            cpu.pc = cpu.memory.read(cpu.sp);
            cpu.pc |= (cpu.memory.read(cpu.sp + 1) << 8);
            cpu.sp += 2;
            return 0;
        }
        return 1;
    };

    // RM: 0xF8 — pris: 11, non-pris: 5
    instructions[0xF8] = [](CPU8080& cpu) -> uint8_t {
        if(cpu.flags.isSign()) {
            cpu.pc = cpu.memory.read(cpu.sp);
            cpu.pc |= (cpu.memory.read(cpu.sp + 1) << 8);
            cpu.sp += 2;
            return 0;
        }
        return 1;
    };

    // RP: 0xF0 — pris: 11, non-pris: 5
    instructions[0xF0] = [](CPU8080& cpu) -> uint8_t {
        if(!cpu.flags.isSign()) {
            cpu.pc = cpu.memory.read(cpu.sp);
            cpu.pc |= (cpu.memory.read(cpu.sp + 1) << 8);
            cpu.sp += 2;
            return 0;
        }
        return 1;
    };

    // RPE: 0xE8 — pris: 11, non-pris: 5
    instructions[0xE8] = [](CPU8080& cpu) -> uint8_t {
        if(cpu.flags.isParity()) {
            cpu.pc = cpu.memory.read(cpu.sp);
            cpu.pc |= (cpu.memory.read(cpu.sp + 1) << 8);
            cpu.sp += 2;
            return 0;
        }
        return 1;
    };

    // RPO: 0xE0 — pris: 11, non-pris: 5
    instructions[0xE0] = [](CPU8080& cpu) -> uint8_t {
        if(!cpu.flags.isParity()) {
            cpu.pc = cpu.memory.read(cpu.sp);
            cpu.pc |= (cpu.memory.read(cpu.sp + 1) << 8);
            cpu.sp += 2;
            return 0;
        }
        return 1;
    };

    // ==================== RST INSTRUCTIONS — BUG #3 FIX: push pc+1, BUG #4 FIX: T-states (11) ====================
    instructions[0xC7] = [](CPU8080& cpu) -> uint8_t { // RST 0
        uint16_t retAddr = cpu.pc + 1;
        cpu.sp -= 2;
        cpu.memory.write(cpu.sp + 1, static_cast<uint8_t>(retAddr >> 8));
        cpu.memory.write(cpu.sp, static_cast<uint8_t>(retAddr & 0xFF));
        cpu.pc = 0x00;
        return 0;
    };
    instructions[0xCF] = [](CPU8080& cpu) -> uint8_t { // RST 1
        uint16_t retAddr = cpu.pc + 1;
        cpu.sp -= 2;
        cpu.memory.write(cpu.sp + 1, static_cast<uint8_t>(retAddr >> 8));
        cpu.memory.write(cpu.sp, static_cast<uint8_t>(retAddr & 0xFF));
        cpu.pc = 0x08;
        return 0;
    };
    instructions[0xD7] = [](CPU8080& cpu) -> uint8_t { // RST 2
        uint16_t retAddr = cpu.pc + 1;
        cpu.sp -= 2;
        cpu.memory.write(cpu.sp + 1, static_cast<uint8_t>(retAddr >> 8));
        cpu.memory.write(cpu.sp, static_cast<uint8_t>(retAddr & 0xFF));
        cpu.pc = 0x10;
        return 0;
    };
    instructions[0xDF] = [](CPU8080& cpu) -> uint8_t { // RST 3
        uint16_t retAddr = cpu.pc + 1;
        cpu.sp -= 2;
        cpu.memory.write(cpu.sp + 1, static_cast<uint8_t>(retAddr >> 8));
        cpu.memory.write(cpu.sp, static_cast<uint8_t>(retAddr & 0xFF));
        cpu.pc = 0x18;
        return 0;
    };
    instructions[0xE7] = [](CPU8080& cpu) -> uint8_t { // RST 4
        uint16_t retAddr = cpu.pc + 1;
        cpu.sp -= 2;
        cpu.memory.write(cpu.sp + 1, static_cast<uint8_t>(retAddr >> 8));
        cpu.memory.write(cpu.sp, static_cast<uint8_t>(retAddr & 0xFF));
        cpu.pc = 0x20;
        return 0;
    };
    instructions[0xEF] = [](CPU8080& cpu) -> uint8_t { // RST 5
        uint16_t retAddr = cpu.pc + 1;
        cpu.sp -= 2;
        cpu.memory.write(cpu.sp + 1, static_cast<uint8_t>(retAddr >> 8));
        cpu.memory.write(cpu.sp, static_cast<uint8_t>(retAddr & 0xFF));
        cpu.pc = 0x28;
        return 0;
    };
    instructions[0xF7] = [](CPU8080& cpu) -> uint8_t { // RST 6
        uint16_t retAddr = cpu.pc + 1;
        cpu.sp -= 2;
        cpu.memory.write(cpu.sp + 1, static_cast<uint8_t>(retAddr >> 8));
        cpu.memory.write(cpu.sp, static_cast<uint8_t>(retAddr & 0xFF));
        cpu.pc = 0x30;
        return 0;
    };
    instructions[0xFF] = [](CPU8080& cpu) -> uint8_t { // RST 7
        uint16_t retAddr = cpu.pc + 1;
        cpu.sp -= 2;
        cpu.memory.write(cpu.sp + 1, static_cast<uint8_t>(retAddr >> 8));
        cpu.memory.write(cpu.sp, static_cast<uint8_t>(retAddr & 0xFF));
        cpu.pc = 0x38;
        return 0;
    };

    // ==================== INTERRUPT INSTRUCTIONS ====================
    // EI (Enable Interrupts): opcode 0xFB — 4 T-states
    instructions[0xFB] = [](CPU8080& cpu) -> uint8_t { 
        cpu.inte = true; 
        // Si une interruption est en attente, la traiter immédiatement
        if (cpu.interruptPending && cpu.intLatch >= 0) {
            cpu.handleInterrupts();
        }
        return 1; 
    };
    // DI (Disable Interrupts): opcode 0xF3 — 4 T-states
    instructions[0xF3] = [](CPU8080& cpu) -> uint8_t { 
        cpu.inte = false; 
        return 1; 
    };

    // ==================== I/O INSTRUCTIONS — BUG #4 FIX: T-states (10) ====================
    instructions[0xDB] = [](CPU8080& cpu) -> uint8_t {  // IN
        uint8_t port = cpu.memory.read(cpu.pc + 1);
        cpu.regs.a = cpu.ioTransfer(port, 0, false);
        return 2;
    };

    instructions[0xD3] = [](CPU8080& cpu) -> uint8_t {  // OUT
        uint8_t port = cpu.memory.read(cpu.pc + 1);
        cpu.ioTransfer(port, cpu.regs.a, true);
        return 2;
    };

    // ==================== NOPs NON DOCUMENTÉS ====================
    instructions[0x08] = [](CPU8080& cpu) -> uint8_t { return 1; };
    instructions[0x10] = [](CPU8080& cpu) -> uint8_t { return 1; };
    instructions[0x18] = [](CPU8080& cpu) -> uint8_t { return 1; };
    instructions[0x20] = [](CPU8080& cpu) -> uint8_t { return 1; };
    instructions[0x28] = [](CPU8080& cpu) -> uint8_t { return 1; };
    instructions[0x30] = [](CPU8080& cpu) -> uint8_t { return 1; };
    instructions[0x38] = [](CPU8080& cpu) -> uint8_t { return 1; };
}

void CPU8080::step() {
    if (halted && !interruptPending) return;

    handleInterrupts();

    uint8_t opcode = memory.read(pc);
    auto instr = instructions[opcode];

    if (!instr) {
        halted = true;
        return;
    }

    // BUG #2 FIX: initialiser currentInstructionTStates à 4 par défaut (reg-reg ALU)
    currentInstructionTStates = 4;
    
    uint8_t bytesConsumed = instr(*this);
    totalCycles += currentInstructionTStates;
    
    pc += bytesConsumed;
}

// ==================== HELPER FUNCTIONS ====================

uint8_t CPU8080::fetch8() {
    return memory.read(pc++);
}

uint16_t CPU8080::fetch16() {
    uint8_t lo = memory.read(pc++);
    uint8_t hi = memory.read(pc++);
    return (static_cast<uint16_t>(hi) << 8) | lo;
}

void CPU8080::push16(uint16_t val) {
    sp -= 2;
    memory.write(sp + 1, static_cast<uint8_t>(val >> 8));
    memory.write(sp, static_cast<uint8_t>(val & 0xFF));
}

uint16_t CPU8080::pop16() {
    uint8_t lo = memory.read(sp);
    uint8_t hi = memory.read(sp + 1);
    sp += 2;
    return (static_cast<uint16_t>(hi) << 8) | lo;
}

// ==================== INTERRUPT HANDLING ====================

void CPU8080::handleInterrupts() {
    // BUG #5 FIX: Interruptions RST 1 & RST 2 — injection correcte
    if (inte && interruptPending && intLatch >= 0) {
        // Désactiver INTE automatiquement à l'entrée de l'interruption (spec Intel 8080)
        inte = false;
        halted = false;
        interruptPending = false;
        
        // PUSH PC sur la pile (SP -= 2, écrire HI puis LO)
        push16(pc);
        
        // Saut au vecteur d'interruption : PC ← intLatch * 8
        pc = static_cast<uint16_t>(intLatch * 8);
        intLatch = -1;
        
        // Coût : 11 T-states (PUSH=11, JUMP=10) — mais le push est inclus dans les 11
        totalCycles += 11;
    }
}

void CPU8080::triggerInterrupt(int vector) {
    if (vector < 0 || vector > 7) return;
    
    // Space Invaders nécessite des interruptions VBLANK cycliques
    // Ne pas bloquer si déjà en attente — re-trigger pour garantir le timing
    interruptPending = true;
    intLatch = vector;
}

// ==================== I/O HANDLING — BUG #2 FIX: null checks ====================

void CPU8080::setIOHandler(uint8_t port, std::function<uint8_t(uint8_t)> reader, std::function<void(uint8_t)> writer) {
    ioHandlers_[port] = {reader, writer};
}

uint8_t CPU8080::ioTransfer(uint8_t port, uint8_t value, bool isOut) {
    auto it = ioHandlers_.find(port);
    if (it != ioHandlers_.end()) {
        if (isOut) {
            // BUG #2 FIX: vérifier null avant d'appeler le handler
            if (it->second.second) it->second.second(value);
        } else {
            if (it->second.first) return it->second.first(value);
        }
    }
    return isOut ? 0 : 0;
}