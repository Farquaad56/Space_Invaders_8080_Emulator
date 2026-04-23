#ifndef INPUT_HPP
#define INPUT_HPP

// Standard library headers must come first (before any third-party headers)
#include <cstdint>
#include <functional>
#include <map>

/**
 * Gestion des Entrées/Sorties Space Invaders (Taito, 1978)
 * 
 * Ports de lecture :
 * - Port 0 : Inutilisé
 * - Port 1 : Crédits, Start joueurs, Tir/Gauche/Droite P1
 * - Port 2 : Vies, Tilt, Bonus, Tir/Gauche/Droite P2, Coin Info
 * - Port 3 : Shift Register Result (géré par shift_register.hpp)
 * 
 * Ports d'écriture :
 * - Port 2 : Shift Amount (offset décalage)
 * - Port 3 : Son Banque 1 (UFO, Tir, Mort joueur, Alien)
 * - Port 4 : Shift Data (données registre)
 * - Port 5 : Son Banque 2 (Pas aliens, UFO Hit)
 */

class InputSystem {
public:
    InputSystem();

    // ==================== PORTS DE LECTURE ====================

    /**
     * Port 1 Read - Entrées joueur 1 et crédits
     * Bit 0 : P2 Credit (1 = Coin inséré)
     * Bit 1 : P2 Start
     * Bit 2 : P1 Start
     * Bit 3 : Toujours 1
     * Bit 4 : P1 Tir
     * Bit 5 : P1 Gauche
     * Bit 6 : P1 Droite
     * Bit 7 : Inutilisé
     */
    uint8_t readPort1();

    /**
     * Port 2 Read - Vies, Tilt, Bonus, Entrées joueur 2
     * Bit 0-1 : Vies (00=3, 01=4, 10=5, 11=6)
     * Bit 2 : Tilt
     * Bit 3 : Bonus (0=1500 pts, 1=1000 pts)
     * Bit 4 : P2 Tir
     * Bit 5 : P2 Gauche
     * Bit 6 : P2 Droite
     * Bit 7 : Coin Info (DIP Switch)
     */
    uint8_t readPort2();

    // ==================== PORTS D'ÉCRITURE ====================

    /**
     * Port 2 Write - Shift Amount (définit l'offset du shift register)
     * @param data Valeur 8 bits (seuls les 3 premiers bits utilisés)
     */
    void writePort2(uint8_t data);

    /**
     * Port 3 Write - Son Banque 1
     * Détection de front montant (rising edge) pour déclenchement des sons
     * @param data Nouvelle valeur du port
     * @param oldData Valeur précédente du port (pour détection edge)
     * Bit 0 : UFO (soucoupe) - Boucle
     * Bit 1 : Tir (Fire)
     * Bit 2 : Mort du joueur (Flash)
     * Bit 3 : Mort d'un alien
     * Bit 4 : Inutilisé
     */
    void writePort3(uint8_t data, uint8_t oldData);

    /**
     * Port 5 Write - Son Banque 2
     * Détection de front montant (rising edge) pour déclenchement des sons
     * @param data Nouvelle valeur du port
     * @param oldData Valeur précédente du port (pour détection edge)
     * Bit 0 : Pas alien 1
     * Bit 1 : Pas alien 2
     * Bit 2 : Pas alien 3
     * Bit 3 : Pas alien 4
     * Bit 4 : UFO Hit (Soucoupe détruite)
     * Bit 5 : Inutilisé
     */
    void writePort5(uint8_t data, uint8_t oldData);

    /**
     * Réinitialiser l'état audio (pour reset émulateur)
     */
    void resetAudio();

    /**
     * Initialisation immédiate du système audio (appelée au démarrage de l'émulateur)
     */
    static void initAudioEarly();

    // ==================== MAPPING CLAVIER ====================

    /**
     * Configurer le mapping clavier pour les contrôles joueur
     */
    void setKeyboardMapping(
        std::function<bool()> p1_fire,
        std::function<bool()> p1_left,
        std::function<bool()> p1_right,
        std::function<bool()> p2_fire = nullptr,
        std::function<bool()> p2_left = nullptr,
        std::function<bool()> p2_right = nullptr
    );

    /**
     * Configurer le mapping clavier pour les boutons Start P1/P2
     */
    void setStartMapping(
        std::function<bool()> p1_start,
        std::function<bool()> p2_start);

    /**
     * Mettre à jour les entrées (à appeler dans la boucle principale)
     */
    void update();

    // ==================== PARAMÈTRES JEU ====================

    void setCredits(uint8_t credits);
    uint8_t getCredits() const { return credits_; }
    void setVies(uint8_t vies);  // 0=3, 1=4, 2=5, 3=6
    void setBonus(bool bonus);   // true = 1500 pts, false = 1000 pts

    // ==================== CONTRÔLES DIRECTS (pour clavier Raylib) ====================

    /**
     * Définir l'état direct d'un bouton (true=pressé, false=re lâché)
     */
    void setKeyState(uint8_t key, bool state);

    /**
     * Déclencher le Start P1 (impulsion)
     */
    void triggerStartP1();

    /**
     * Définir l'état d'une key par son code (alias pour setKeyState)
     * Utilisé par emulator.cpp pour connecter CPU IN instruction → input states
     */
    void setInputKeyState(uint8_t key, bool state);

    // ==================== AUDIO CALLBACKS ====================

    /**
     * Configurer un callback audio pour un port de son
     */
    using AudioCallback = std::function<void(uint8_t)>;
    void setAudioCallback(uint8_t port, AudioCallback cb);

private:
    // État des entrées joueur 1
    std::function<bool()> p1_fire_;
    std::function<bool()> p1_left_;
    std::function<bool()> p1_right_;

    // État des entrées joueur 2 (optionnel)
    std::function<bool()> p2_fire_;
    std::function<bool()> p2_left_;
    std::function<bool()> p2_right_;

    // Boutons Start P1/P2
    std::function<bool()> p1_start_;
    std::function<bool()> p2_start_;

    // Paramètres de jeu
    uint8_t credits_;
    uint8_t vies_;       // Valeur encodée (0-3)
    bool bonus_;         // true = 1500 pts, false = 1000 pts

    // État des boutons (accès direct pour clavier Raylib)
    bool p1_fire_state_;
    bool p1_left_state_;
    bool p1_right_state_;
    bool p2_fire_state_;
    bool p2_left_state_;
    bool p2_right_state_;
    bool p1_start_state_;
    bool p2_start_state_;

    // Callbacks audio externes
    AudioCallback audio_callback_3_;
    AudioCallback audio_callback_5_;
};

#endif // INPUT_HPP