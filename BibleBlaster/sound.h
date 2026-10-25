// Music and sound effects, using ArduboyPlaytune: the music plays on one speaker pin and the
// sound effects on the other, so shooting never cuts the song off. All tunes are public domain.
// Scores: 0x90 n = start note n (MIDI, 60 = middle C), 0x80 = stop, two bytes = wait in ms,
// 0xE0 = start again, 0xF0 = end.
#pragma once
#include <ArduboyPlaytune.h>
ArduboyPlaytune tunes(ab.audio.enabled);
// "Amazing Grace" (tune New Britain, 1835): title and menu screens
const uint8_t SONG_GRACE[] PROGMEM = {144,62,1,124,128,0,40,144,67,3,32,128,0,40,144,71,0,170,128,0,40,144,67,0,170,128,0,40,144,71,3,32,128,0,40,144,69,1,124,128,0,40,144,67,3,32,128,0,40,144,64,1,124,128,0,40,144,62,3,32,128,0,40,144,62,1,124,128,0,40,144,67,3,32,128,0,40,144,71,0,170,128,0,40,144,67,0,170,128,0,40,144,71,3,32,128,0,40,144,69,1,124,128,0,40,144,74,8,12,128,0,40,144,71,1,124,128,0,40,144,74,3,32,128,0,40,144,71,0,170,128,0,40,144,67,0,170,128,0,40,144,71,3,32,128,0,40,144,69,1,124,128,0,40,144,67,3,32,128,0,40,144,64,1,124,128,0,40,144,62,3,32,128,0,40,144,62,1,124,128,0,40,144,67,3,32,128,0,40,144,71,0,170,128,0,40,144,67,0,170,128,0,40,144,71,3,32,128,0,40,144,69,1,124,128,0,40,144,67,8,12,128,0,40,4,236,224};
// "When the Saints Go Marching In" (traditional spiritual): while you play
const uint8_t SONG_SAINTS[] PROGMEM = {144,60,1,64,128,0,40,144,64,1,64,128,0,40,144,65,1,64,128,0,40,144,67,5,120,128,0,40,1,104,144,60,1,64,128,0,40,144,64,1,64,128,0,40,144,65,1,64,128,0,40,144,67,5,120,128,0,40,1,104,144,60,1,64,128,0,40,144,64,1,64,128,0,40,144,65,1,64,128,0,40,144,67,2,168,128,0,40,144,64,2,168,128,0,40,144,60,2,168,128,0,40,144,64,2,168,128,0,40,144,62,6,224,128,0,40,1,104,144,64,1,64,128,0,40,144,64,1,64,128,0,40,144,62,1,64,128,0,40,144,60,4,16,128,0,40,144,60,1,64,128,0,40,144,64,2,168,128,0,40,144,67,2,168,128,0,40,144,67,1,64,128,0,40,144,65,5,120,128,0,40,1,104,144,64,1,64,128,0,40,144,65,1,64,128,0,40,144,67,2,168,128,0,40,144,64,2,168,128,0,40,144,60,2,168,128,0,40,144,62,2,168,128,0,40,144,60,6,224,128,0,40,4,56,224};
// "Nearer, My God, to Thee" (tune Bethany, Lowell Mason, 1856): game over
const uint8_t SONG_NEARER[] PROGMEM = {144,71,3,232,128,0,40,144,69,1,224,128,0,40,144,67,3,232,128,0,40,144,67,1,224,128,0,40,144,64,3,232,128,0,40,144,62,1,224,128,0,40,144,62,5,240,128,0,40,144,67,1,224,128,0,40,144,71,3,232,128,0,40,144,69,7,248,128,0,40,2,8,144,71,3,232,128,0,40,144,69,1,224,128,0,40,144,67,3,232,128,0,40,144,67,1,224,128,0,40,144,64,3,232,128,0,40,144,62,1,224,128,0,40,144,62,5,240,128,0,40,144,67,1,224,128,0,40,144,69,3,232,128,0,40,144,67,10,0,128,0,40,240};
// "Hallelujah!" (Handel, Messiah, 1741): the SECOND COMING
const uint8_t SONG_HALLELUJAH[] PROGMEM = {144,74,1,44,128,0,40,144,69,0,215,128,0,40,144,71,0,68,128,0,17,144,69,1,44,128,0,40,0,170,144,74,1,44,128,0,40,144,69,0,215,128,0,40,144,71,0,68,128,0,17,144,69,1,44,128,0,40,0,170,144,69,0,136,128,0,34,144,74,0,215,128,0,40,144,76,0,68,128,0,17,144,78,1,44,128,0,40,144,76,0,136,128,0,34,144,74,2,128,128,0,40,240};

void soundBegin() { tunes.initChannel(PIN_SPEAKER_1); tunes.initChannel(PIN_SPEAKER_2); }
void playSong(const uint8_t* s) { tunes.playScore(s); }
// effects: a few (Hz, frames) steps played on the second speaker pin, one step per frame tick
const uint16_t* fx; uint8_t fxN, fxT;
const uint16_t FX_SHOOT[]  PROGMEM = {1200, 2, 900, 2, 650, 2, 450, 2};   // whoosh as the bible leaves your hand
const uint16_t FX_EMPTY[]  PROGMEM = {150, 3, 0, 3, 120, 3};              // dull double click: nothing to throw
const uint16_t FX_RELOAD[] PROGMEM = {700, 1, 0, 2, 1000, 2};             // click-clack, one per bible
const uint16_t FX_CLICK[]  PROGMEM = {900, 3};
void play(const uint16_t* f, uint8_t steps) { fx = f; fxN = steps; fxT = 0; }
#define sndShoot()  play(FX_SHOOT, 4)
#define sndEmpty()  play(FX_EMPTY, 3)
#define sndReload() play(FX_RELOAD, 3)
#define sndClick()  play(FX_CLICK, 1)
void updateSound() {
  if (!fxN || fxT--) return;
  uint16_t hz = pgm_read_word(fx), fr = pgm_read_word(fx + 1); fx += 2; fxN--;
  fxT = fr - 1;
  if (hz) tunes.tone(hz, fr * 17);
}
