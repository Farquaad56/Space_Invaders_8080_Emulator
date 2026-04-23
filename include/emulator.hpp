#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include "cpu8080.hpp"
#include "shift_register.hpp"
#include "video.hpp"
#include "input.hpp"
#include <cstdint>
#include <vector>
#include <string>
#include <chrono>

/**
 * Emulateur Space Invaders (Taito, 1978)
 * 
 * Orchestrateur principal qui combine :
 * - CPU Intel 8080A à 1.9968 MHz
 * - Shift Register Fujitsu MB14241 (ports 2,3,4)
 * - VRAM 0x2400-0x3FFF (7 Ko, rotation 90° anti-horaire)
 * - Entrées/Sorties (ports 1-5)
 * - Interruptions VBLANK (RST 1 à mi-écran, RST 2 en fin d'écran)
 */

class Emulator {
public:
    // Fréquence CPU Space Invaders : 1.9968 MHz
    static constexpr double CPU_FREQ_HZ = 1996800.0;
    static constexpr uint32_t TARGET_CYCLES_PER_FRAME = 
        static_cast<uint32_t>(CPU_FREQ_HZ / 60.0);  // ~33280 T-States par frame à 60 Hz

    // Durée cible d'une frame en ms pour throttle temps réel (1000/60 ≈ 16ms)
    static constexpr long long TARGET_FRAME_MS = 16;

    Emulator();
    ~Emulator();

    /**
     * Charger les ROMs Space Invaders (4 fichiers de 2 Ko chacun)
     * @param pathH Chemin vers invaders.h (adresse 0x0000)
     * @param pathG Chemin vers invaders.g (adresse 0x0800)
     * @param pathF Chemin vers invaders.f (adresse 0x1000)
     * @param pathE Chemin vers invaders.e (adresse 0x1800)
     * @return true si chargement réussi
     */
    bool loadROMs(
        const std::string& pathH,
        const std::string& pathG,
        const std::string& pathF,
        const std::string& pathE
    );

    /**
     * Initialiser l'émulateur (connecter les handlers I/O)
     */
    void initialize();

    /**
     * Réinitialiser l'émulateur
     */
    void reset();

    /**
     * Exécuter une frame complète (60 Hz)
     * @return true si emulation continue, false si arrêt demandé
     */
    bool runFrame();

    /**
     * Mettre en pause / reprendre
     */
    void setPaused(bool paused) { paused_ = paused; }
    bool isPaused() const { return paused_; }
    bool isRunning() const { return running_; }

    /**
     * Vitesse d'émulation (0.5x, 1.0x, 2.0x)
     */
    void setEmulationSpeed(float speed) { emulation_speed_ = speed; }
    float getEmulationSpeed() const { return emulation_speed_; }

    /**
     * Accesseurs pour le GUI
     */
    CPU8080& getCpu() { return cpu_; }
    VideoSystem& getVideo() { return video_; }
    ShiftRegister& getShiftRegister() { return shift_register_; }
    InputSystem& getInput() { return input_; }

    /**
     * Générer le framebuffer pour affichage (rotation 90° anti-horaire)
     */
    void generateFramebuffer(uint8_t* framebuffer, int& width, int& height);

    /**
     * Gérer les interruptions VBLANK
     * @param vector Vecteur d'interruption (1 = mi-écran, 2 = fin d'écran)
     */
    void triggerVBlankInterrupt(int vector);

    /**
     * Définir le nombre de crédits
     */
    void setCredits(uint8_t credits) { input_.setCredits(credits); }
    uint8_t getCredits() const { return input_.getCredits(); }

    // Direct input control from GUI
    void setInputKeyState(uint8_t key, bool state) { input_.setInputKeyState(key, state); }
    void triggerStartP1() { input_.triggerStartP1(); }
    
    /**
     * BUG-02 v2 FIX: Déclencher impulsion credit (coin) pour 1 frame
     */
    void triggerCoin() { input_.triggerCoin(); }
    
    /**
     * BUG-03 v2 FIX: Mettre à jour les états d'impulsion (reset Start P1, Coin)
     */
    void updateInput() { input_.updateInput(); }

private:
    CPU8080 cpu_;
    ShiftRegister shift_register_;
    VideoSystem video_;
    InputSystem input_;

    bool paused_;
    bool running_;
    
    // Vitesse d'émulation (0.5x, 1.0x, 2.0x)
    float emulation_speed_;
    
    // Flags pour déclenchement automatique VBLANK par frame
    bool vblank1Triggered_{false};
    bool vblank2Triggered_{false};

    // Valeurs précédentes des ports I/O pour détection de front montant (rising edge) audio
    uint8_t oldPort3_{0x00};  // Port 3 son banque 1
    uint8_t oldPort5_{0x00};  // Port 5 son banque 2

    // Synchronisation temps réel 60 Hz
    std::chrono::steady_clock::time_point lastFrameStart_;

    /**
     * Configurer les handlers I/O du CPU pour mapper vers le hardware Space Invaders
     */
    void setupIOHandlers();

    /**
     * Handler pour lecture port 1 (entrées joueur 1 + crédits)
     */
    uint8_t ioReadPort1(uint8_t);

    /**
     * Handler pour lecture port 2 (vies, tilt, bonus + entrées joueur 2)
     */
    uint8_t ioReadPort2(uint8_t);

    /**
     * Handler pour écriture port 2 (shift amount)
     */
    void ioWritePort2(uint8_t data);

    /**
     * Handler pour écriture port 3 (son banque 1)
     */
    void ioWritePort3(uint8_t data);

    /**
     * Handler pour écriture port 4 (shift data)
     */
    void ioWritePort4(uint8_t data);

    /**
     * Handler pour écriture port 5 (son banque 2)
     */
    void ioWritePort5(uint8_t data);

    /**
     * Handler pour écriture port 6 — Graphismes/Shifted Data (Space Invaders)
     * Le port 0x06 est utilisé par la ROM pour écrire des données shiftées en VRAM
     */
    void ioWritePort6(uint8_t data);

    // Valeurs précédentes des ports I/O pour détection de front montant
    uint8_t oldPort6_{0x00};  // Port 6 graphismes/shifted data
};

#endif // EMULATOR_HPP