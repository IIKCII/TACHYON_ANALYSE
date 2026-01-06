/*
 * Teensy DS4 Soundboard Engine
 * Spielt Audio-Samples über USB Audio Interface ab
 * 22050 Hz, 16-bit Mono PCM
 */

#ifndef SOUNDBOARD_H
#define SOUNDBOARD_H

#include <Arduino.h>

// Audio-Konfiguration (muss mit USB Descriptors übereinstimmen!)
#define SOUNDBOARD_SAMPLE_RATE  22050  // Hz
#define SOUNDBOARD_CHANNELS     1      // Mono
#define SOUNDBOARD_BIT_DEPTH    16     // 16-bit signed PCM
#define SOUNDBOARD_PACKET_SIZE  90     // Bytes pro 1ms USB Packet
#define SOUNDBOARD_SAMPLES_PER_PACKET (SOUNDBOARD_PACKET_SIZE / 2)  // 45 samples/packet

// Maximale Anzahl von Sounds
#define SOUNDBOARD_MAX_SOUNDS   20

// Sound-Slot IDs
enum SoundID {
    SOUND_NONE = 0,
    SOUND_KAIDO_LAUGH = 1,
    // Weitere Sounds hier hinzufügen...
    SOUND_MAX
};

// Audio-Sample Struktur
struct AudioSample {
    const int16_t* data;     // Zeiger auf PROGMEM Audio-Daten
    uint32_t length;         // Anzahl Samples
    const char* name;        // Name für Debug
};

// Soundboard-Engine Klasse
class Soundboard {
public:
    Soundboard();

    // Initialisierung
    void begin();

    // Sound-Registrierung
    void registerSound(SoundID id, const int16_t* data, uint32_t length, const char* name);

    // Playback-Kontrolle
    void play(SoundID id);
    void stop();
    bool isPlaying() const { return playing_; }

    // USB Audio Callback (wird von USB ISR aufgerufen!)
    // Füllt buffer mit 45 samples (90 bytes)
    void fillAudioBuffer(int16_t* buffer, uint32_t num_samples);

    // Status-Info für Debug
    SoundID getCurrentSound() const { return current_sound_; }
    uint32_t getPlaybackPosition() const { return playback_position_; }
    float getPlaybackProgress() const;

private:
    // Sound-Datenbank
    AudioSample sounds_[SOUNDBOARD_MAX_SOUNDS];

    // Playback-State
    bool playing_;
    SoundID current_sound_;
    uint32_t playback_position_;  // Aktuelle Sample-Position

    // Hilfsfunktionen
    AudioSample* getSound(SoundID id);
};

// Globale Soundboard-Instanz
extern Soundboard soundboard;

#endif // SOUNDBOARD_H
