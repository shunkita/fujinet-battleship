/*
  Platform specific sound functions for Commodore 64
*/

#include <stdint.h>
#include <stdlib.h>
#include <peekpoke.h>
#include "../misc.h"
#include "../platform-specific/sound.h"
#include "../platform-specific/util.h"

// C64 SID chip registers
#define SID_BASE 0xD400

#define SID_FREQ_LO 0xD400
#define SID_FREQ_HI 0xD401
#define SID_PW_LO 0xD402
#define SID_PW_HI 0xD403
#define SID_CONTROL 0xD404
#define SID_ATTACK_DECAY 0xD405
#define SID_SUSTAIN_RELEASE 0xD406

#define SID_FILTER_FREQ_LO 0xD415
#define SID_FILTER_FREQ_HI 0xD416
#define SID_FILTER_RES_FILT 0xD417
#define SID_FILTER_MODE_VOL 0xD418

// Gate bit in control register
#define GATE_ON 0x01
#define GATE_OFF 0x00

// Waveform bits
#define WAVEFORM_TRIANGLE 0x10
#define WAVEFORM_SAWTOOTH 0x20
#define WAVEFORM_PULSE 0x40
#define WAVEFORM_NOISE 0x80

uint16_t ii;

#define SID_CLOCK 985248UL

// Convert a frequency in Hz to a 16-bit SID frequency word (Fn)
// Formula from SID docs: Fout = Fn * Fclk / 16777216
// Therefore Fn = Fout * 16777216 / Fclk
// Returns 0 on input 0, clamps to 0xFFFF on overflow.
// Implemented without 64-bit math (cc65) by factoring 16777216 = 65536 * 256.
uint16_t hz_to_sidfn(uint16_t hz)
{
    uint32_t fn;
    uint32_t numerator;
    uint32_t denominator;

    if (hz == 0) return 0;

    // Fn = (hz * 65536) / (Fclk / 256)
    // This keeps the intermediate within 32 bits for typical audio frequencies.
    numerator = (uint32_t)hz * 65536UL;
    denominator = SID_CLOCK / 256UL;
    if (denominator == 0) return 0;

    fn = numerator / denominator;
    if (fn > 0xFFFFU) return 0xFFFFU;
    return (uint16_t)fn;
}

void wait_ms(uint16_t ms)
{
    uint32_t target_us;
    uint32_t elapsed;
    uint16_t last;
    uint16_t now;
    uint16_t delta;
    
    if (ms == 0) return;

    target_us = (uint32_t)ms * 1000U;

    resetTimer();
    elapsed = 0;
    last = getTime();

    while (elapsed < target_us) {
        now = getTime();
        delta = now - last; // unsigned wrap-around safe
        elapsed += delta;
        last = now;
    }
}

void initSound()
{
    POKE(SID_FILTER_MODE_VOL, 0x0F);  // Set volume to max, no filter
}

void playToneVoice(uint8_t voiceIndex, uint16_t frequency, uint8_t duration, uint8_t waveform, uint16_t pulseWidth, uint8_t attack_decay, uint8_t sustain_release, uint8_t extra_ctrl_bits)
{
    uint32_t base = SID_BASE + (uint32_t)voiceIndex * 7;
    uint16_t sidfn = hz_to_sidfn(frequency);
    uint16_t pw12 = pulseWidth & 0x0FFF;

    if (voiceIndex > 2) return;
    if (frequency == 0 && waveform == 0) return;

    POKE(base + 0, sidfn & 0xFF); // FREQ LO
    POKE(base + 1, sidfn >> 8);   // FREQ HI
    POKE(base + 2, pw12 & 0xFF);  // PW LO
    POKE(base + 3, (pw12 >> 8) & 0x0F); // PW HI (only low 4 bits used)
    POKE(base + 5, attack_decay);
    POKE(base + 6, sustain_release);

    // Trigger the envelope
    POKE(base + 4, waveform | GATE_ON | extra_ctrl_bits);

    wait_ms(duration);

    // Stop (begin release)
    POKE(base + 4, waveform & ~GATE_ON);
}

// Stop any playing sound (clears all voice gates)
void soundStop()
{
    POKE(SID_BASE + 4, 0);
    POKE(SID_BASE + 7 + 4, 0);
    POKE(SID_BASE + 14 + 4, 0);
}

static uint8_t keySoundsEnabled = 1;

void soundCursor()
{
    if (prefs.disableSound || !keySoundsEnabled)
        return;

    // small blip
    playToneVoice(0, 900, 40, WAVEFORM_PULSE, 0x0800, 0x12, 0x02, 0x00);
}

void soundSelect()
{
    if (prefs.disableSound || !keySoundsEnabled)
        return;

    // slightly sharper blip
    playToneVoice(0, 1400, 50, WAVEFORM_PULSE, 0x0700, 0x11, 0x02, 0x00);
}

void soundJoinGame()
{
    if (prefs.disableSound)
        return;

    // three short rising notes
    playToneVoice(0, 700, 80, WAVEFORM_SAWTOOTH, 0x000, 0x14, 0x03, 0x00);
    playToneVoice(0, 900, 80, WAVEFORM_SAWTOOTH, 0x000, 0x14, 0x03, 0x00);
    playToneVoice(0, 1100, 140, WAVEFORM_SAWTOOTH, 0x000, 0x14, 0x04, 0x00);
}

void soundMyTurn()
{
    if (prefs.disableSound)
        return;

    // Ring-modulated gong-like short tone
    // Use RING bit (0x04) with triangle waveform
    playToneVoice(0, 250, 300, WAVEFORM_TRIANGLE, 0x000, 0x08, 0x06, 0x04);
}

void soundGameDone()
{
    if (prefs.disableSound)
        return;

    // Long descending tone
    playToneVoice(0, 600, 220, WAVEFORM_SAWTOOTH, 0x000, 0x06, 0x05, 0x00);
    playToneVoice(0, 350, 400, WAVEFORM_SAWTOOTH, 0x000, 0x06, 0x07, 0x00);
}

void soundTick()
{
    if (prefs.disableSound)
        return;

    // clock tick: very short click
    playToneVoice(0, 1200, 30, WAVEFORM_TRIANGLE, 0x000, 0x01, 0x01, 0x00);
}

void soundPlaceShip()
{
    if (prefs.disableSound)
        return;

    // plucky attack
    playToneVoice(0, 500, 90, WAVEFORM_PULSE, 0x0600, 0x03, 0x04, 0x00);
}

void soundAttack()
{
    if (prefs.disableSound)
        return;

    // heavy gunshot: low thump with short decay
    playToneVoice(0, 80, 70, WAVEFORM_NOISE, 0x000, 0x03, 0x03, 0x00);
}

void soundInvalid()
{
    if (prefs.disableSound)
        return;

    // low buzzy reject tone
    playToneVoice(0, 220, 120, WAVEFORM_SAWTOOTH, 0x000, 0x01, 0x03, 0x00);
}

void soundHit()
{
    if (prefs.disableSound)
        return;

    // explosion: layered noise bursts
    playToneVoice(0, 60, 200, WAVEFORM_NOISE, 0x000, 0x05, 0x0F, 0x00);
    playToneVoice(0, 30, 400, WAVEFORM_NOISE, 0x000, 0x07, 0x0F, 0x00);
}

void soundSink()
{
    // TODO: A bigger sound for a sunken shup
    soundHit();
}

void soundMiss()
{
    if (prefs.disableSound)
        return;

    // water splash - lower noise with quick decay
    playToneVoice(0, 600, 100, WAVEFORM_NOISE, 0x000, 0x03, 0x04, 0x00);
}

void disableKeySounds()
{
    keySoundsEnabled = 0;
}

void enableKeySounds()
{
    keySoundsEnabled = 1;
}
