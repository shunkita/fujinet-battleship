/*
  Platform specific sound functions for Commodore 64
*/

#include <stdint.h>
#include <stdlib.h>
#include <peekpoke.h>
#include "../misc.h"
#include "../platform-specific/sound.h"

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

void initSound()
{
    POKE(SID_FILTER_MODE_VOL, 0x0F);  // Set volume to max, no filter
}

// More capable play function: frequency is 16-bit SID tone value
void playToneFull(uint16_t frequency, uint8_t duration, uint8_t waveform, uint16_t pulseWidth, uint8_t attack_decay, uint8_t sustain_release)
{
    if (prefs.disableSound)
        return;

    // Set frequency (16-bit)
    POKE(SID_FREQ_LO, frequency & 0xFF);
    POKE(SID_FREQ_HI, (frequency >> 8) & 0xFF);

    // Set pulse width (12-bit) - low and high parts
    POKE(SID_PW_LO, pulseWidth & 0xFF);
    POKE(SID_PW_HI, (pulseWidth >> 8) & 0x0F);

    // Set envelope (ADSR)
    POKE(SID_ATTACK_DECAY, attack_decay);
    POKE(SID_SUSTAIN_RELEASE, sustain_release);

    // Gate on with requested waveform
    POKE(SID_CONTROL, waveform | GATE_ON);

    // Play for duration (coarse frames)
    while (duration--)
        for (ii = 0; ii < 100; ii++)
            ;

    // Gate off (stop sound)
    POKE(SID_CONTROL, waveform);
}

// Simple wrapper kept for backward compatibility with existing callers.
void playTone(uint16_t frequency, uint8_t duration, uint8_t waveform)
{
    // Defaults: moderate pulse width, fast attack, medium decay/release
    playToneFull(frequency, duration, waveform, 0x0800, 0xF4, 0x44);
}

void soundCursor()
{
    if (prefs.disableSound)
        return;

    // short pulse clicks (narrow pulsewidth)
    playToneFull(100, 2, WAVEFORM_PULSE, 0x0400, 0xF6, 0x22);
    playToneFull(120, 2, WAVEFORM_PULSE, 0x0400, 0xF6, 0x22);
    playToneFull(140, 2, WAVEFORM_PULSE, 0x0400, 0xF6, 0x22);
}

void soundSelect()
{
    if (prefs.disableSound)
        return;

    playTone(150, 3, WAVEFORM_PULSE);
}

void soundStop()
{
    // Stop any playing sound
    POKE(SID_CONTROL, 0);
}

void soundJoinGame()
{
    if (prefs.disableSound)
        return;

    // Cheerful short melody using triangle with gentle envelope
    playToneFull(80, 2, WAVEFORM_TRIANGLE, 0, 0x63, 0x54);
    playToneFull(60, 2, WAVEFORM_TRIANGLE, 0, 0x63, 0x54);
    playToneFull(100, 3, WAVEFORM_TRIANGLE, 0, 0x63, 0x54);
}

void soundMyTurn()
{
    if (prefs.disableSound)
        return;

    playTone(120, 1, WAVEFORM_PULSE);
    playTone(120, 2, WAVEFORM_PULSE);
}

void soundGameDone()
{
    uint8_t i;
    if (prefs.disableSound)
        return;

    for (i = 0; i < 3; i++)
    {
        playTone(200, 2, WAVEFORM_TRIANGLE);
        playTone(100, 2, WAVEFORM_TRIANGLE);
    }
    playTone(150, 5, WAVEFORM_TRIANGLE);
}

void soundTick()
{
    if (prefs.disableSound)
        return;

    playTone(160, 1, WAVEFORM_PULSE);
}

void soundPlaceShip()
{
    uint8_t i;
    if (prefs.disableSound)
        return;

    // Small arpeggio with pulse -> triangle mix
    for (i = 0; i < 5; i++)
    {
        playToneFull(100 + (i * 8), 1, WAVEFORM_PULSE, 0x0600, 0x84, 0x33);
    }
}

void soundAttack()
{
    uint8_t i;
    if (prefs.disableSound)
        return;

    // Whoosh / attack: descending sawtooth with quick decay
    for (i = 200; i > 100; i -= 10)
    {
        playToneFull(i, 1, WAVEFORM_SAWTOOTH, 0, 0xE4, 0x33);
    }
}

void soundInvalid()
{
    if (prefs.disableSound)
        return;

    // Short dissonant buzzer using narrow pulsewidth and fast envelope
    playToneFull(120, 1, WAVEFORM_PULSE, 0x0100, 0xF8, 0x11);
    playToneFull(90, 1, WAVEFORM_PULSE, 0x0200, 0xF8, 0x11);
    playToneFull(70, 1, WAVEFORM_PULSE, 0x0300, 0xF8, 0x11);
}

void soundHit()
{
    uint8_t i;
    uint8_t prevFilter;
    if (prefs.disableSound)
        return;

    // Explosion / hit: layered noise with falling tone and punchy envelope
    // Increase resonance/lowpass to get a boom-like noise.
    // Temporarily tweak filter mode/volume register to emphasize low end.
    prevFilter = PEEK(SID_FILTER_MODE_VOL);
    // Set filter mode to lowpass with some volume (keep top nibble as mode, low nibble volume)
    POKE(SID_FILTER_MODE_VOL, (prevFilter & 0xF0) | 0x0F);

    for (i = 0; i < 6; i++)
    {
        // Start bright and fall down
        playToneFull(300 - (i * 40), 1, WAVEFORM_NOISE, 0, 0xF6, 0x44);
    }

    // restore filter/volume
    POKE(SID_FILTER_MODE_VOL, prevFilter);
}

void soundMiss()
{
    if (prefs.disableSound)
        return;

    playTone(80, 2, WAVEFORM_PULSE);
    playTone(60, 2, WAVEFORM_PULSE);
}

void disableKeySounds()
{
}

void enableKeySounds()
{
}
