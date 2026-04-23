#ifndef ALU_HPP
#define ALU_HPP

#include <cstdint>
#include "flags.hpp"

class ALU {
public:
    static bool parity(uint8_t v) {
        int p = 0;
        uint8_t x = v;
        while (x) {
            p ^= (x & 1);
            x >>= 1;
        }
        return p == 0;
    }

    static void updateCommonFlags(uint8_t res, Flags& flags) {
        uint8_t val = res;
        flags.setZero(val == 0);
        flags.setSign((val & 0x80) != 0);
        flags.setParity(parity(val));
    }

    static uint8_t add(uint8_t a, uint8_t b, Flags& flags) {
        uint16_t res = static_cast<uint16_t>(a) + b;
        uint8_t val = static_cast<uint8_t>(res & 0xFF);
        updateCommonFlags(val, flags);
        flags.setCarry((res > 0xFF));
        flags.setAuxiliaryCarry(((a & 0x0F) + (b & 0x0F)) > 0x0F);
        return val;
    }

    static uint8_t sub(uint8_t a, uint8_t b, Flags& flags) {
        // SUB A,B : res = a - b, CY=1 si borrow (a < b), AC=1 si emprunt bit 3→4
        uint16_t res = static_cast<uint16_t>(static_cast<int>(a) - static_cast<int>(b));
        uint8_t val = static_cast<uint8_t>(res & 0xFF);
        updateCommonFlags(val, flags);
        // CY=1 si emprunt (borrow) : a < b
        flags.setCarry(a < b);
        // AC=1 si emprunt du bit 3 au bit 4 (Auxiliary Carry)
        // Correcte selon spec Intel 8080 : (a & 0x0F) < ((b & 0x0F))
        flags.setAuxiliaryCarry((a & 0x0F) < (b & 0x0F));
        return val;
    }

    static uint8_t adc(uint8_t a, uint8_t b, Flags& flags) {
        uint16_t carryIn = flags.isCarry() ? 1 : 0;
        uint16_t res = static_cast<uint16_t>(a) + b + carryIn;
        uint8_t val = static_cast<uint8_t>(res & 0xFF);
        updateCommonFlags(val, flags);
        flags.setCarry((res > 0xFF));
        flags.setAuxiliaryCarry(((a & 0x0F) + (b & 0x0F) + carryIn) > 0x0F);
        return val;
    }

    static uint8_t sbb(uint8_t a, uint8_t b, Flags& flags) {
        // SBB A,B : res = a - b - CY, CY=1 si borrow, AC=1 si emprunt bit 3→4
        int carryIn = flags.isCarry() ? 1 : 0;
        int resInt = static_cast<int>(a) - static_cast<int>(b) - carryIn;
        uint8_t val = static_cast<uint8_t>(resInt & 0xFF);
        updateCommonFlags(val, flags);
        // CY=1 si borrow global : a < b + carryIn
        flags.setCarry(static_cast<unsigned int>(a) < static_cast<unsigned int>(b + carryIn));
        // AC=1 si emprunt bit 3→4 : (a & 0x0F) < ((b & 0x0F) + carryIn)
        flags.setAuxiliaryCarry((static_cast<unsigned int>(a & 0x0F)) < (static_cast<unsigned int>(b & 0x0F) + carryIn));
        return val;
    }

    static uint8_t and_op(uint8_t a, uint8_t b, Flags& flags) {
        uint8_t res = a & b;
        updateCommonFlags(res, flags);
        flags.setCarry(false);
        flags.setAuxiliaryCarry(true); // ANA/ANI must set AC=1 per spec (often used for BCD)
        return res;
    }

    static uint8_t or_op(uint8_t a, uint8_t b, Flags& flags) {
        uint8_t res = a | b;
        updateCommonFlags(res, flags);
        flags.setCarry(false);
        flags.setAuxiliaryCarry(false); // ORA/ORI must clear AC per Intel spec
        return res;
    }

    static uint8_t xor_op(uint8_t a, uint8_t b, Flags& flags) {
        uint8_t res = a ^ b;
        updateCommonFlags(res, flags);
        flags.setCarry(false);
        flags.setAuxiliaryCarry(false); // XRA/XRI must clear AC per Intel spec
        return res;
    }

    static uint8_t cmp(uint8_t a, uint8_t b, Flags& flags) {
        // CMP A,B : effectue a - b sans modifier A, CY=1 si borrow (a < b), AC=1 si emprunt bit 3→4
        int resInt = static_cast<int>(a) - static_cast<int>(b);
        uint8_t val = static_cast<uint8_t>(resInt & 0xFF);
        updateCommonFlags(val, flags);
        // CY=1 si borrow (borrow flag, pas carry) : a < b
        flags.setCarry(a < b);
        // AC=1 si emprunt bit 3→4 : (a & 0x0F) < (b & 0x0F)
        flags.setAuxiliaryCarry((static_cast<unsigned int>(a & 0x0F)) < (static_cast<unsigned int>(b & 0x0F)));
        // CMP ne modifie pas A — retourne a inchangé
        return a;
    }

    static uint8_t rlc(uint8_t a, Flags& flags) {
        // RLC A : rotate left circular, CY ← bit7, bit7 ← bit6
        bool bit7 = (a & 0x80) != 0;
        uint8_t res = ((static_cast<unsigned int>(a) << 1) | (a >> 7)) & 0xFF;
        flags.setCarry(bit7);
        updateCommonFlags(res, flags);
        return res;
    }

    static uint8_t rrc(uint8_t a, Flags& flags) {
        // RRC A : rotate right circular, CY ← bit0, bit0 ← bit7
        bool bit0 = (a & 0x01) != 0;
        uint8_t res = ((static_cast<unsigned int>(a) >> 1) | (a << 7)) & 0xFF;
        flags.setCarry(bit0);
        updateCommonFlags(res, flags);
        return res;
    }

    static uint8_t ral(uint8_t a, Flags& flags) {
        // RAL A : rotate left through carry, CY ← bit7, bit7 ← old CY
        bool carryIn = flags.isCarry();
        bool bit7 = (a & 0x80) != 0;
        uint8_t res = ((static_cast<unsigned int>(a) << 1) | (carryIn ? 1 : 0)) & 0xFF;
        flags.setCarry(bit7);
        updateCommonFlags(res, flags);
        return res;
    }

    static uint8_t rar(uint8_t a, Flags& flags) {
        // RAR A : rotate right through carry, CY ← bit0, bit0 ← old CY
        bool carryIn = flags.isCarry();
        bool bit0 = (a & 0x01) != 0;
        uint8_t res = ((static_cast<unsigned int>(carryIn ? 0x80 : 0)) | (a >> 1)) & 0xFF;
        flags.setCarry(bit0);
        updateCommonFlags(res, flags);
        return res;
    }
};

#endif // ALU_HPP