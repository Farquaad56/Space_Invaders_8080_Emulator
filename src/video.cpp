#include "video.hpp"
#include <cstring>

VideoSystem::VideoSystem() 
    : vram_(VRAM_SIZE, 0), 
      on_r_(0), on_g_(255), on_b_(255),  // Cyan #00FFFF
      off_r_(0), off_g_(0), off_b_(0) {   // Noir #000000
    // VRAM initialisée à zéro
}

uint8_t VideoSystem::readVRAM(uint16_t addr) const {
    if (addr >= VRAM_START && addr < VRAM_START + VRAM_SIZE) {
        return vram_[addr - VRAM_START];
    }
    return 0;
}

void VideoSystem::writeVRAM(uint16_t addr, uint8_t data) {
    if (addr >= VRAM_START && addr < VRAM_START + VRAM_SIZE) {
        vram_[addr - VRAM_START] = data;
    }
}

void VideoSystem::setOnColor(uint8_t r, uint8_t g, uint8_t b) {
    on_r_ = r;
    on_g_ = g;
    on_b_ = b;
}

void VideoSystem::setOffColor(uint8_t r, uint8_t g, uint8_t b) {
    off_r_ = r;
    off_g_ = g;
    off_b_ = b;
}

void VideoSystem::generateFramebuffer(uint8_t* framebuffer, int& width, int& height) {
    // Dimensions après rotation 90° anti-horaire : 224 x 256
    width = SCREEN_WIDTH;   // 224
    height = SCREEN_HEIGHT; // 256
    
    // Initialiser tout le framebuffer avec la couleur "off" (noir)
    memset(framebuffer, 0, width * height * 4);
    
    // Format VRAM Space Invaders :
    // - La VRAM est "couchée" en mémoire (rotation 90° anti-horaire)
    // - Chaque octet = 8 pixels verticaux (bit 0 = bas, bit 7 = haut)
    // - Les octets sont organisés par bandes de 32 octets (stride)
    // - VRAM_SIZE / 32 = 7168 / 32 = 224 bandes = hauteur logique de l'écran original
    // 
    // Mapping :
    // vram_y (0..223) → position X sur écran moderne (colonnes)
    // vram_x (0..31)  → groupe de 8 pixels en Y
    // bit (0..7)      → pixel individuel dans le groupe
    
    for (int vram_y = 0; vram_y < VRAM_SIZE / 32; ++vram_y) {       // 0..223 (bandes/colonnes)
        for (int vram_x = 0; vram_x < 32; ++vram_x) {               // 0..31 (groupes de 8 pixels Y)
            uint8_t byte = vram_[vram_y * 32 + vram_x];
            
            for (int bit = 0; bit < 8; ++bit) {
                int pixel_on = (byte >> bit) & 1;
                
                // Position sur écran moderne (rotation 90° anti-horaire) :
                // X = position de la bande verticale (0..223)
                // Y = inversé pour que les aliens soient tête en haut
                // bit 7 (tête) → SCREEN_HEIGHT-1 (haut), bit 0 (pieds) → 0 (bas)
                int screen_x = vram_y;                                          // 0..223
                int screen_y = SCREEN_HEIGHT - 1 - (vram_x * 8 + bit);         // 255..0
                
                if (screen_x >= 0 && screen_x < width && screen_y >= 0 && screen_y < height) {
                    int idx = (screen_y * width + screen_x) * 4;
                    if (pixel_on) {
                        framebuffer[idx + 0] = on_r_;
                        framebuffer[idx + 1] = on_g_;
                        framebuffer[idx + 2] = on_b_;
                        framebuffer[idx + 3] = 255;
                    } else {
                        framebuffer[idx + 0] = off_r_;
                        framebuffer[idx + 1] = off_g_;
                        framebuffer[idx + 2] = off_b_;
                        framebuffer[idx + 3] = 255;
                    }
                }
            }
        }
    }
}
