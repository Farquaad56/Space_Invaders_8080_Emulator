#ifndef SHIFT_REGISTER_HPP
#define SHIFT_REGISTER_HPP

#include <cstdint>

/**
 * Fujitsu MB14241 Shift Register - Hardware Space Invaders (Taito, 1978)
 * 
 * Ce circuit externe permet de décaler rapidement des sprites sans utiliser le CPU.
 * Le 8080 n'a pas d'instruction de décalage multiple, donc Taito a ajouté cette puce.
 * 
 * Ports utilisés :
 * - Port 2 (Write) : Définit l'offset de décalage (3 bits, 0-7)
 * - Port 4 (Write) : Écrit les données dans le registre 16 bits
 * - Port 3 (Read)  : Renvoie le résultat du décalage de 8 bits
 */

class ShiftRegister {
public:
    ShiftRegister();

    /**
     * Port 2 Write - Définit l'offset de décalage
     * @param data Valeur 8 bits (seuls les 3 premiers bits utilisés)
     */
    void writePort2(uint8_t data);

    /**
     * Port 4 Write - Écrit dans le registre de décalage 16 bits
     * Logique : Registre = (Nouvelle_Donnée << 8) | (Ancienne_Donnée >> 8)
     * @param data Valeur 8 bits à écrire dans la partie haute
     */
    void writePort4(uint8_t data);

    /**
     * Port 3 Read - Lit le résultat du décalage de 8 bits
     * @return Résultat du décalage : (Registre >> (8 - offset)) & 0xFF
     */
    uint8_t readPort3();

    /**
     * Accesseur — Offset de décalage actuel
     * @return Valeur de l'offset (0-7)
     */
    uint8_t getOffset() const { return shift_offset_; }

    /**
     * Accesseur — Valeur du registre 16 bits
     * @return Contenu du registre
     */
    uint16_t getValue() const { return shift_register_; }

 private:
    uint16_t shift_register_;  // Registre 16 bits
    uint8_t  shift_offset_;    // Offset de décalage (3 bits, 0-7)
};

#endif // SHIFT_REGISTER_HPP