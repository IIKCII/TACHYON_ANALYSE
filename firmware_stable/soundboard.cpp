/*
 * Teensy DS4 Soundboard Engine - Implementierung
 */

#include "soundboard.h"

// Globale Soundboard-Instanz
Soundboard soundboard;

// ============================================================================
// Konstruktor
// ============================================================================
Soundboard::Soundboard()
    : playing_(false)
    , current_sound_(SOUND_NONE)
    , playback_position_(0)
{
    // Sound-Datenbank initialisieren
    for (int i = 0; i < SOUNDBOARD_MAX_SOUNDS; i++) {
        sounds_[i].data = nullptr;
        sounds_[i].length = 0;
        sounds_[i].name = nullptr;
    }
}

// ============================================================================
// Initialisierung
// ============================================================================
void Soundboard::begin() {
    Serial.println("[Soundboard] Initialisiere...");

    // Playback-State zurücksetzen
    playing_ = false;
    current_sound_ = SOUND_NONE;
    playback_position_ = 0;

    Serial.println("[Soundboard] Bereit!");
}

// ============================================================================
// Sound registrieren
// ============================================================================
void Soundboard::registerSound(SoundID id, const int16_t* data, uint32_t length, const char* name) {
    if (id >= SOUNDBOARD_MAX_SOUNDS) {
        Serial.printf("[Soundboard] FEHLER: Sound ID %d ungültig!\n", id);
        return;
    }

    sounds_[id].data = data;
    sounds_[id].length = length;
    sounds_[id].name = name;

    float duration = (float)length / SOUNDBOARD_SAMPLE_RATE;
    Serial.printf("[Soundboard] Sound #%d registriert: %s (%.2fs, %d samples)\n",
                  id, name, duration, length);
}

// ============================================================================
// Sound abspielen
// ============================================================================
void Soundboard::play(SoundID id) {
    AudioSample* sound = getSound(id);

    if (!sound || !sound->data) {
        Serial.printf("[Soundboard] Sound #%d nicht gefunden!\n", id);
        return;
    }

    // Stoppe aktuellen Sound
    if (playing_) {
        stop();
    }

    // Starte neuen Sound
    current_sound_ = id;
    playback_position_ = 0;
    playing_ = true;

    Serial.printf("[Soundboard] Spiele: %s\n", sound->name);
}

// ============================================================================
// Sound stoppen
// ============================================================================
void Soundboard::stop() {
    if (playing_) {
        Serial.printf("[Soundboard] Stoppe: %s\n", sounds_[current_sound_].name);
    }

    playing_ = false;
    current_sound_ = SOUND_NONE;
    playback_position_ = 0;
}

// ============================================================================
// Playback-Fortschritt (0.0 - 1.0)
// ============================================================================
float Soundboard::getPlaybackProgress() const {
    if (!playing_ || current_sound_ == SOUND_NONE) {
        return 0.0f;
    }

    AudioSample* sound = const_cast<Soundboard*>(this)->getSound(current_sound_);
    if (!sound || sound->length == 0) {
        return 0.0f;
    }

    return (float)playback_position_ / sound->length;
}

// ============================================================================
// USB Audio Buffer füllen (WIRD VON USB ISR AUFGERUFEN!)
// ============================================================================
void Soundboard::fillAudioBuffer(int16_t* buffer, uint32_t num_samples) {
    // Wenn kein Sound läuft: Stille senden
    if (!playing_) {
        memset(buffer, 0, num_samples * sizeof(int16_t));
        return;
    }

    // Hole aktuellen Sound
    AudioSample* sound = getSound(current_sound_);
    if (!sound || !sound->data) {
        // Fehler: Sound nicht gefunden
        playing_ = false;
        memset(buffer, 0, num_samples * sizeof(int16_t));
        return;
    }

    // Audio-Daten aus PROGMEM kopieren
    for (uint32_t i = 0; i < num_samples; i++) {
        if (playback_position_ < sound->length) {
            // Sample aus Flash lesen (PROGMEM!)
            buffer[i] = pgm_read_word(&sound->data[playback_position_]);
            playback_position_++;
        } else {
            // Ende des Sounds erreicht
            buffer[i] = 0;
            playing_ = false;
            current_sound_ = SOUND_NONE;
            playback_position_ = 0;
        }
    }
}

// ============================================================================
// Sound holen (Hilfsfunktion)
// ============================================================================
AudioSample* Soundboard::getSound(SoundID id) {
    if (id >= SOUNDBOARD_MAX_SOUNDS) {
        return nullptr;
    }
    return &sounds_[id];
}
