#include "shift_register.hpp"

ShiftRegister::ShiftRegister() : shift_register_(0), shift_offset_(0) {
    // Initialisation à zéro
}

void ShiftRegister::writePort2(uint8_t data) {
    // Seul les 3 premiers bits sont utilisés (0-7)
    shift_offset_ = data & 0x07;
}

void ShiftRegister::writePort4(uint8_t data) {
    // Logique de décalage : nouvelle donnée dans partie haute, ancienne vers partie basse
    shift_register_ = (static_cast<uint16_t>(data) << 8) | (shift_register_ >> 8);
}

uint8_t ShiftRegister::readPort3() {
    // BUG #1 FIX: La formule générale couvre offset=0 : shift_register_ >> 8 = octet haut
    uint16_t result = shift_register_ >> (8 - shift_offset_);
    return static_cast<uint8_t>(result & 0xFF);
}
