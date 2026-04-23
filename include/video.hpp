#ifndef VIDEO_HPP
#define VIDEO_HPP

#include <cstdint>
#include <vector>
#include <functional>

/**
 * Système Vidéo Space Invaders (Taito, 1978)
 * 
 * - Résolution : 256 x 224 pixels
 * - VRAM : 0x2400 - 0x3FFF (7 Ko = 7168 octets)
 * - Orientation : Verticale (Portrait)
 * - Format mémoire : "couché" en mémoire, rotation 90° anti-horaire nécessaire
 * - Couleurs : Monochrome noir/blanc → cyan (#00FFFF) sur écran moderne
 * 
 * Chaque bit représente un pixel (1 = Allumé, 0 = Éteint).
 * L'octet à 0x2400 correspond aux 8 premiers pixels en bas à gauche de l'écran physique.
 */

class VideoSystem {
public:
    static constexpr uint16_t VRAM_START = 0x2400;
    static constexpr uint16_t VRAM_SIZE = 7168;  // 7 Ko
    // BUG #5 FIX: Space Invaders physique = 256x224 portrait, après rotation 90° CCW = 224x256
    static constexpr int SCREEN_WIDTH = 224;
    static constexpr int SCREEN_HEIGHT = 256;

    VideoSystem();

    /**
     * Lire un octet de la VRAM
     */
    uint8_t readVRAM(uint16_t addr) const;

    /**
     * Écrire un octet dans la VRAM
     */
    void writeVRAM(uint16_t addr, uint8_t data);

    /**
     * Accéder à la mémoire VRAM brute (pour le CPU via memory.read/write)
     */
    const std::vector<uint8_t>& getVRAM() const { return vram_; }
    std::vector<uint8_t>& getVRAMRef() { return vram_; }

    /**
     * Générer le framebuffer pour affichage moderne (rotation 90° anti-horaire)
     * @param framebuffer Tableau RGBA de taille SCREEN_WIDTH * SCREEN_HEIGHT * 4
     * @param width Sortie : largeur du framebuffer
     * @param height Sortie : hauteur du framebuffer
     */
    void generateFramebuffer(uint8_t* framebuffer, int& width, int& height);

    /**
     * Définir la couleur des pixels "allumés" (par défaut cyan #00FFFF)
     */
    void setOnColor(uint8_t r, uint8_t g, uint8_t b);

    /**
     * Définir la couleur des pixels "éteints" (par défaut noir #000000)
     */
    void setOffColor(uint8_t r, uint8_t g, uint8_t b);

private:
    std::vector<uint8_t> vram_;  // VRAM 7 Ko

    // Couleurs personnalisables
    uint8_t on_r_, on_g_, on_b_;  // Couleur pixel allumé (default: cyan)
    uint8_t off_r_, off_g_, off_b_;  // Couleur pixel éteint (default: noir)
};

#endif // VIDEO_HPP