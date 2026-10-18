// BILLY GRAHAM'S BIBLE BLASTER  -  Arduboy
// by jaychugstein
//
// Heathens walk across the street. Aim with the arrows (left/right to turn,
// up/down to lob nearer or farther) and fire bibles to convert them.
// Outer (right) button fires. Inner (left) button reloads.
#include <Arduboy2.h>
#include <EEPROM.h>
#include "title.h"
#include "sprites.h"
#include "font.h"
#include "bg.h"
#include "ctrl.h"

Arduboy2Base ab;

#define BTN_FIRE B_BUTTON   // outer button
#define BTN_LOAD A_BUTTON   // inner button
#define ANY_BTN (UP_BUTTON | DOWN_BUTTON | LEFT_BUTTON | RIGHT_BUTTON | A_BUTTON | B_BUTTON)
#define EE_BEST 80

enum { M_TITLE, M_BOOM, M_PLAY, M_OVER, M_COMING, M_VERSE, M_CTRL };
uint8_t mode = M_TITLE;
uint16_t tick = 0;

// ---------------------------------------------------------------- drawing
const uint8_t BIT[8] PROGMEM = {1, 2, 4, 8, 16, 32, 64, 128};
void pset(int16_t x, int16_t y) {
  if ((uint16_t)x > 127 || (uint16_t)y > 63) return;
  ab.sBuffer[(y >> 3) * 128 + x] |= pgm_read_byte(&BIT[y & 7]);
}
void pclr(int16_t x, int16_t y) {
  if ((uint16_t)x > 127 || (uint16_t)y > 63) return;
  ab.sBuffer[(y >> 3) * 128 + x] &= ~pgm_read_byte(&BIT[y & 7]);
}
void col8(int16_t x, int16_t y, uint8_t b) {
  if ((uint16_t)x > 127 || !b) return;
  if (y < 0) { if (y <= -8) return; b >>= -y; y = 0; }
  if (y > 63) return;
  uint8_t* d = &ab.sBuffer[(y >> 3) * 128 + x]; uint8_t sh = y & 7;
  d[0] |= b << sh; if (sh && y < 56) d[128] |= b >> (8 - sh);
}
// big 5x7 text
void txt(int16_t x, int16_t y, const char* s) {
  for (; *s; s++, x += 6) { uint8_t c = *s; uint8_t g = (c < 32 || c > 90) ? 0 : c - 32; for (uint8_t i = 0; i < 5; i++) col8(x + i, y, pgm_read_byte(&FONT[g * 5 + i])); }
}
// tiny 3x5 text
const uint8_t TINY[177] PROGMEM = {0,0,0,0,23,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,4,14,4,16,8,0,4,4,4,0,16,0,24,4,3,31,17,31,18,31,16,25,21,18,17,21,10,7,4,31,23,21,9,30,21,29,1,29,3,31,21,31,23,21,15,0,10,0,0,0,0,0,0,0,0,0,0,0,0,0,1,21,3,0,0,0,30,5,30,31,21,10,14,17,17,31,17,14,31,21,17,31,5,1,14,17,29,31,4,31,17,31,17,8,16,15,31,4,27,31,16,16,31,6,31,31,1,30,14,17,14,31,5,2,14,25,30,31,5,26,18,21,9,1,31,1,31,16,31,15,16,15,31,12,31,27,4,27,3,28,3,25,21,19};
int16_t tiny(int16_t x, int16_t y, const char* s) {
  for (; *s; s++, x += 4) { uint8_t c = *s; if (c < 32 || c > 90) c = 32; for (uint8_t i = 0; i < 3; i++) col8(x + i, y, pgm_read_byte(&TINY[(c - 32) * 3 + i])); }
  return x;
}
// fast black rectangle straight in the screen buffer
void black(int16_t x, int16_t y, int16_t w, int16_t h) {
  if (x < 0) { w += x; x = 0; } if (y < 0) { h += y; y = 0; }
  if (x + w > 128) w = 128 - x; if (y + h > 64) h = 64 - y;
  if (w <= 0 || h <= 0) return;
  for (int16_t yy = y; yy < y + h; ) {
    uint8_t page = yy >> 3, top = yy & 7, n = min(8 - top, y + h - yy);
    uint8_t m = ~(((1 << n) - 1) << top);
    uint8_t* d = &ab.sBuffer[page * 128 + x];
    for (int16_t i = 0; i < w; i++) d[i] &= m;
    yy += n;
  }
}

// ---------------------------------------------------------------- title + explosion
void drawTitle() {
  memcpy_P(ab.sBuffer, TITLE, 1024);
  if (tick & 32) for (uint8_t x = PRESS_X; x < PRESS_X + PRESS_W; x++) ab.sBuffer[7 * 128 + x] &= 0x01;
}
// Every lit pixel of the title flies outward and falls. Paths come from each
// pixel's own position, so nothing is stored. Runs at 30fps (2000+ pixels).
#define BOOM_LEN 24
void drawBoom() {
  int16_t t = tick * 2, fall = (t * t) >> 4;
  const uint8_t* src = TITLE;
  for (uint8_t p = 0; p < 8; p++) for (uint8_t x = 0; x < 128; x++) {
    uint8_t b = pgm_read_byte(src++);
    if (!b) continue;
    for (uint8_t r = 0; r < 8; r++, b >>= 1) {
      if (!(b & 1)) continue;
      uint8_t y = p * 8 + r;
      uint16_t h = (uint16_t)(x * 211 + y * 97) * 40503u;
      int8_t sp = 3 + ((h >> 12) & 7);
      int16_t vx = ((int16_t)(x - 64) * sp >> 3) + (int8_t)((h & 31) - 16);
      int16_t vy = ((int16_t)(y - 32) * sp >> 3) + (int8_t)(((h >> 5) & 31) - 16) - 20;
      pset(x + ((vx * t) >> 4), y + ((vy * t) >> 4) + fall);
    }
  }
}

// ---------------------------------------------------------------- the game
#define HORIZON 21
// three walking lanes: far, middle, near
const uint8_t LANE_Y[3] = {32, 40, 52};   // feet line
const uint8_t LANE_AIM[3] = {23, 28, 35};   // aim height: middle of a walker's body
const uint8_t LANE_DUR[3] = {22, 16, 11};   // bible flight time in frames
const uint8_t LANE_HW[3] = {7, 9, 12};      // half body width
const uint8_t LANE_SPD[3] = {4, 6, 9};    // walking speed, 1/16 px per frame

enum { T_CAVE, T_SUIT, T_UNIT, T_BAHAI };
struct Walker { int16_t x16; uint8_t on, lane, type, dir, spd, step; };
#define NW 6
Walker wk[NW];

struct Bible { uint8_t on, t, dur, lane; int16_t tx, ty; };
#define NB 5
Bible bb[NB];

struct Poof { uint8_t t; int16_t x, y; };
#define NP 4
Poof pf[NP];

int16_t aimX16, aimY16;      // crosshair, 1/16 px (Y glides to the chosen lane)
uint8_t aimLane;             // up/down picks which lane the bible lands in
uint8_t holdT;               // frames until the next bible is up in your hand (0 = ready)
#define RAISE_T 14           // how long a new bible takes to come up
uint8_t ammo, loading, fireWait;
uint8_t lives, converts, misses;
uint16_t totalConv, totalPart, totalThrown;   // for the end screen: full conversions, partial (Unitarians made)
uint16_t bonusT;   // frames left in the SECOND COMING bonus game
#define CONVERTS_FOR_BONUS 10
#define BONUS_LEN (15 * 60)
uint32_t score, best;
uint16_t spawnT, playT;
const char* msg; uint8_t msgT;
int8_t cloudX[2] = {20, 80};

const char M_CONVERT[] PROGMEM = "CONVERT THE HEATHENS";
const char M_KEEP[] PROGMEM = "KEEP FIRING";
const char M_GOT[] PROGMEM = "GOT HIM";
const char M_WING[] PROGMEM = "YOU JUST WINGED HIM AND MADE HIM A UNITARIAN";
const char M_BAHAI[] PROGMEM = "LOOK OUT! A GENTLE BAHAI";
const char M_FULL[] PROGMEM = "ALL RIGHT! FULL CONVERSION";
const char M_SECOND[] PROGMEM = "SECOND COMING!";
const char M_RELOAD[] PROGMEM = "RELOAD RELOAD";
const char M_DOUBLE[] PROGMEM = "DOUBLE CONVERSION";
const char M_LAST[] PROGMEM = "LAST BIBLE!";
const char M_BACK[] PROGMEM = "HE'LL BE BACK";

#define MSG_LONG 150
void say(const char* m) {
  if (msg == M_LAST && msgT > 50 && m != M_RELOAD) return;   // let LAST BIBLE! be read before anything replaces it
  msg = m; msgT = strlen_P(m) > 29 ? MSG_LONG : 100;
}

// Draw a sprite stored as rows of (white bits, mask bits), straight into the screen buffer.
// Drawn inverted (white bits become ink) because the play screen is flipped to daytime at the end.
void blit(const uint32_t* d, uint8_t W, uint8_t H, int16_t x0, int16_t y0, bool mirror) {
  for (uint8_t r = 0; r < H; r++, d += 2) {
    int16_t y = y0 + r; if ((uint16_t)y > 63) continue;
    uint8_t* row = &ab.sBuffer[(y >> 3) * 128]; uint8_t bit = pgm_read_byte(&BIT[y & 7]), nbit = ~bit;
    const uint8_t* pw = (const uint8_t*)d; const uint8_t* pm = (const uint8_t*)(d + 1);
    for (uint8_t q = 0; q < 4; q++) {
      uint8_t mk = pgm_read_byte(pm + q); if (!mk) continue;
      uint8_t wh = pgm_read_byte(pw + q);
      for (uint8_t c = q * 8; mk; c++, mk >>= 1, wh >>= 1) {
        if (!(mk & 1)) continue;
        int16_t x = mirror ? x0 + W - 1 - c : x0 + c;
        if ((uint16_t)x > 127) continue;
        if (wh & 1) row[x] &= nbit; else row[x] |= bit;
      }
    }
  }
}
void drawWalker(Walker& w) {
  uint8_t s = w.lane, W = SPR_W[s], H = SPR_H[s];
  uint8_t fr = (w.step >> 3) & 1;
  int16_t x0 = (w.x16 >> 4) - W / 2, y0 = LANE_Y[s] - H + 1;
  if (w.type == T_BAHAI) {        // the gentle Bahai floats above the street, bobbing, with a shadow below
    int8_t bob = ((playT >> 4) & 1) ? 1 : 0;
    y0 -= 2 + s + bob; fr = 0;
    for (int8_t k = -2 - s; k <= 2 + s; k += 2) pset((w.x16 >> 4) + k, LANE_Y[s]);
  }
  const uint32_t* d = SPR + pgm_read_word(&SPR_OFF[(s * 4 + w.type) * 2 + fr]);
  blit(d, W, H, x0, y0, !w.dir);   // dir 1 = walking right; walking left is the mirror image
}

void spawn() {
  for (uint8_t i = 0; i < NW; i++) if (!wk[i].on) {
    Walker& w = wk[i];
    w.on = 1; w.lane = random(3); w.dir = random(2); w.step = 0;
    w.type = random(10) == 0 ? T_BAHAI : T_CAVE;
    uint8_t s = LANE_SPD[w.lane] + random(0, LANE_SPD[w.lane] / 2) + min(score / 600, 8);
    if (w.type == T_BAHAI) { s += s / 2; say(M_BAHAI); }
    w.spd = s;
    w.x16 = w.dir ? -12 * 16 : 140 * 16;
    return;
  }
}

void startGame() {
  memset(wk, 0, sizeof(wk)); memset(bb, 0, sizeof(bb)); memset(pf, 0, sizeof(pf));
  aimX16 = 64 * 16; aimLane = 1; holdT = 0; aimY16 = LANE_AIM[1] * 16; ammo = 6; loading = 0; fireWait = 0;
  lives = 5; converts = 0; totalConv = 0; totalPart = 0; totalThrown = 0; bonusT = 0; misses = 0; score = 0; spawnT = 60; playT = 0;
#ifdef TESTBIG
  score = 999999UL; totalConv = 65535; totalPart = 65535; totalThrown = 65535; lives = 5;
#endif
  say(M_CONVERT);
}

void poof(int16_t x, int16_t y) { for (uint8_t i = 0; i < NP; i++) if (!pf[i].t) { pf[i].t = 16; pf[i].x = x; pf[i].y = y; return; } }

// score tops out at 999999 so it always fits its box
void addScore(uint16_t n) { score += n; if (score > 999999UL) score = 999999UL; }

void land(Bible& b) {
  int16_t tx = b.tx >> 4, ty = b.ty >> 4;
  // Find who the bible hit. Anyone whose drawn body covers the landing spot counts, whatever row
  // they walk in (so hitting feet or heads works). In the row you aimed at, being a little off to
  // the side still wings him. Ties go to whoever is nearer to you.
  int8_t bi = -1; uint8_t grade = 0, bestLane = 0;  // grade: 3 full, 2 got, 1 winged
  uint8_t gr[NW];
  for (uint8_t i = 0; i < NW; i++) {
    Walker& w = wk[i]; gr[i] = 0;
    if (!w.on || w.type == T_SUIT) continue;   // cavemen, Unitarians and the Bahai can be hit
    uint8_t L = w.lane, hw = LANE_HW[L];
    int16_t dx = abs(tx - (w.x16 >> 4));
    int16_t lift = w.type == T_BAHAI ? 2 + L : 0;
    int16_t feet = LANE_Y[L] - lift, top = feet - SPR_H[L] + 2;
    uint8_t g = 0;
    if (L == b.lane || (ty >= top && ty <= feet)) {
      if (dx <= hw / 2) g = 3;
      else if (dx <= hw) g = 2;
      else if (L == b.lane && dx <= hw + 4) g = 1;
    }
    gr[i] = g;
    if (g > grade || (g && g == grade && L > bestLane)) { grade = g; bi = i; bestLane = L; }
  }
  if (bi < 0) {
    poof(tx, ty);
    if (++misses >= 2) { if (!msgT) say(M_KEEP); misses = 0; }
    return;
  }
  misses = 0;
  if (grade == 1) {                       // only clipped him: winged, he needs a second hit
    Walker& w = wk[bi];
    poof(w.x16 >> 4, LANE_Y[w.lane] - SPR_H[w.lane] / 2);
    if (w.type == T_CAVE) { w.type = T_UNIT; addScore(25); if (totalPart < 65535) totalPart++; say(M_WING); }
    return;
  }
  // full conversion of everyone the bible really hit (any hit finishes off a Unitarian)
  uint8_t n = 0;
  for (uint8_t i = 0; i < NW; i++) if (gr[i] >= 2) {
    Walker& w = wk[i];
    poof(w.x16 >> 4, LANE_Y[w.lane] - SPR_H[w.lane] / 2);
    addScore((gr[i] == 3 ? 100 : 50) + (w.type == T_BAHAI ? 100 : 0));
    w.type = T_SUIT; w.spd += w.spd / 2;  // converted folks hurry off
    if (totalConv < 65535) totalConv++;
    n++;
    if (!bonusT) converts++;
  }
  if (n >= 2) addScore(200);              // two with one bible
  if (!bonusT && converts >= CONVERTS_FOR_BONUS) {
    converts = 0; bonusT = BONUS_LEN; spawnT = 20; msgT = 0;
    mode = M_COMING; tick = 0;            // big SECOND COMING screen first
  } else say(n >= 2 ? M_DOUBLE : grade == 3 ? M_FULL : M_GOT);
}

#ifdef TESTHIT
// automatic hit test: one standing walker, one bible per case, result kept in testRes
// cases 0..173: lane = k/58, direction = (k/29)%2, aim offset = k%29 - 14
// cases 174..175: walker in front of the aimed lane (near in front of middle, middle in front of far)
volatile uint8_t testRes[179] __attribute__((used)); uint8_t testK = 0, testT = 0;
void testStep() {
  if (testK >= 179) return;
  if (testT == 0) {
    memset(wk, 0, sizeof(wk)); memset(bb, 0, sizeof(bb));
    Walker& w = wk[0]; w.on = 1; w.type = T_CAVE; w.spd = 0; w.x16 = 64 * 16;
    int8_t off; uint8_t aimL;
    if (testK < 174) { w.lane = testK / 58; w.dir = (testK / 29) & 1; off = testK % 29 - 14; aimL = w.lane; }
    else if (testK < 176) { w.lane = testK == 174 ? 2 : 1; w.dir = 1; off = 0; aimL = w.lane - 1; }
    else if (testK < 178) { w.lane = testK == 176 ? 0 : 1; w.dir = 1; off = 0; aimL = w.lane + 1; }   // lands on his legs
    else { w.lane = 1; w.dir = 1; off = 0; aimL = 1; wk[1] = w; wk[1].x16 = 67 * 16; wk[1].dir = 0; }   // two at once
    Bible& b = bb[0]; b.on = 1; b.t = 0; b.lane = aimL; b.tx = (64 + off) * 16; b.ty = LANE_AIM[aimL] * 16; b.dur = LANE_DUR[aimL];
  }
  if (++testT > 30) { testRes[testK] = wk[0].type + (testK == 178 ? wk[1].type * 16 : 0); testK++; testT = 0; }
}
#endif

void updatePlay() {
  playT++;
#ifdef TESTMSG
  { static const char* const ALL[11] = {M_CONVERT, M_KEEP, M_GOT, M_WING, M_BAHAI, M_FULL, M_SECOND, M_RELOAD, M_DOUBLE, M_LAST, M_BACK};
    msg = ALL[(playT / 160) % 11]; msgT = 90; lives = 5; }
#endif
#ifdef TESTHIT
  testStep(); lives = 5; spawnT = 200; bonusT = 0; converts = 0;
  for (uint8_t i = 0; i < NB; i++) if (bb[i].on && ++bb[i].t >= bb[i].dur) { bb[i].on = 0; land(bb[i]); }
  return;
#endif
  // aim
  if (ab.pressed(LEFT_BUTTON)) aimX16 -= 30;
  if (ab.pressed(RIGHT_BUTTON)) aimX16 += 30;
  if (ab.justPressed(UP_BUTTON) && aimLane) aimLane--;          // up = lob farther
  if (ab.justPressed(DOWN_BUTTON) && aimLane < 2) aimLane++;    // down = nearer
  { int16_t ty = LANE_AIM[aimLane] * 16; aimY16 += (ty - aimY16) / 3; if (abs(ty - aimY16) < 3) aimY16 = ty; }
  aimX16 = constrain(aimX16, 6 * 16, 121 * 16);

#ifdef TESTBOT
#ifndef BOT_STOP
#define BOT_STOP 60000
#endif
  bool botFire = false, botLoad = false;
  { static uint8_t cool; if (cool) cool--;
    for (uint8_t i = 0; i < NW; i++) { Walker& w = wk[i];
      if (!w.on || w.type == T_SUIT) continue;
      uint8_t dur = LANE_DUR[w.lane];
      int16_t px = (w.x16 + (w.dir ? 1 : -1) * (int16_t)w.spd * dur) >> 4;
      if (px < 8 || px > 120) continue;
      aimX16 = (px + random(-8, 9)) * 16; aimLane = w.lane;
      if (!cool && playT < BOT_STOP) { botFire = true; cool = 25; }
      break; }
    if (!ammo) botLoad = true; }
#define FIREP (ab.justPressed(BTN_FIRE) || botFire)
#define LOADP (ab.justPressed(BTN_LOAD) || botLoad)
#else
#define FIREP ab.justPressed(BTN_FIRE)
#define LOADP ab.justPressed(BTN_LOAD)
#endif
  // fire: the bible in your hand is thrown, then the next one comes up
  bool inHand = ammo || bonusT;
  if (holdT && inHand) holdT--;
  if (FIREP) {
    if (!inHand) say(M_RELOAD);
    else if (!holdT) {
      for (uint8_t i = 0; i < NB; i++) if (!bb[i].on) {
        Bible& b = bb[i]; b.on = 1; b.t = 0; b.tx = aimX16; b.lane = aimLane; b.ty = LANE_AIM[aimLane] * 16;
        b.dur = LANE_DUR[aimLane];
        holdT = bonusT ? RAISE_T / 2 : RAISE_T; if (totalThrown < 65535) totalThrown++;
        if (!bonusT) { ammo--; loading = 0; if (ammo == 1) say(M_LAST); }
        break;
      }
    }
  }
  // reload, one bible at a time
  if (LOADP && ammo < 6 && !loading) loading = 1;
  if (loading && ++loading > 10) { loading = 1; if (!ammo) holdT = RAISE_T; if (++ammo >= 6) loading = 0; }

  // bibles in flight
  for (uint8_t i = 0; i < NB; i++) if (bb[i].on && ++bb[i].t >= bb[i].dur) { bb[i].on = 0; land(bb[i]); }

  // walkers
  for (uint8_t i = 0; i < NW; i++) {
    Walker& w = wk[i]; if (!w.on) continue;
    w.x16 += w.dir ? w.spd : -w.spd;
    w.step += w.spd >> 2;
    int16_t x = w.x16 >> 4;
    if (x < -14 || x > 142) {
      w.on = 0;
      if ((w.type == T_CAVE || w.type == T_UNIT) && lives && !bonusT) lives--;   // no penalty in the bonus game
    }
  }
  if (!lives) { mode = M_OVER; tick = 0; if (score > best) { best = score; EEPROM.put(EE_BEST, best ^ 0xB1B1E000UL); } return; }

  // spawning gets quicker as the score climbs
  if (!--spawnT) { spawn(); spawnT = bonusT ? 22 + random(0, 18) : max(50, 170 - (int16_t)min(score / 30, 120)) + random(0, 50); }
  if (bonusT && !--bonusT) { ammo = 6; loading = 0; say(M_BACK); }   // bonus over: back to normal with a full load

  for (uint8_t i = 0; i < NP; i++) if (pf[i].t) pf[i].t--;
  if (msgT) msgT--;
  if ((playT & 63) == 0) { cloudX[0]--; if (cloudX[0] < -20) cloudX[0] = 127; }
  if ((playT & 127) == 0) { cloudX[1]--; if (cloudX[1] < -20) cloudX[1] = 127; }
}

// ------------- scene
const uint8_t CLOUD[22] PROGMEM = {0, 0, 48, 72, 68, 68, 68, 66, 66, 65, 65, 65, 65, 66, 66, 68, 68, 68, 72, 48, 0, 0};
void cloud(int16_t x, int16_t y) {
  for (uint8_t i = 0; i < 22; i++) {
    uint8_t b = pgm_read_byte(&CLOUD[i]); if (!b) continue;
    uint8_t lo = 0, hi = 7; while (!(b >> lo & 1)) lo++; while (!(b >> hi & 1)) hi--;
    for (uint8_t r = lo; r <= hi; r++) pclr(x + i, y + r);   // blank sky inside the cloud
    col8(x + i, y, b);
  }
}
void drawScene() {
  memcpy_P(ab.sBuffer, BG, 1024);           // houses, trees, street, lamps and sun
  cloud(cloudX[0], 2); cloud(cloudX[1], 7);
}
// a tumbling bible: s = size (0 huge, 1 big, 2 mid, 3 small), f = frame (0 cover, 1 open)
void drawBible(int16_t cx, int16_t cy, uint8_t s, uint8_t f) {
  uint8_t k = s * 2 + f, W = BIB_W[k], H = BIB_H[k];
  const uint32_t* d = BIB + pgm_read_byte(&BIB_OFF[k]);
  int16_t x0 = cx - W / 2, y0 = cy - H / 2;
  blit(d, W, H, x0, y0, false);
}
// The bible in your hand at the bottom of the screen. After a throw the next one rises into view.
void drawHand() {
  if (!(ammo || bonusT)) return;                           // empty-handed: reload!
  int16_t rise = holdT ? (int16_t)holdT * (BIB_H[0] + 2) / RAISE_T : 0;
  if (bonusT) rise = holdT * (BIB_H[0] + 2) / (RAISE_T / 2);
  drawBible(64, 63 - BIB_H[0] / 2 + 1 + rise, 0, 0);
}
void drawFrame() {
  // octagon screen frame like an arcade cabinet bezel
  const uint8_t c = 7;
  for (uint8_t k = 0; k < 2; k++) {
    ab.drawFastHLine(c, k, 128 - 2 * c, WHITE); ab.drawFastHLine(c, 63 - k, 128 - 2 * c, WHITE);
    ab.drawFastVLine(k, c, 64 - 2 * c, WHITE); ab.drawFastVLine(127 - k, c, 64 - 2 * c, WHITE);
    ab.drawLine(k, c, c, k, WHITE); ab.drawLine(127 - k, c, 127 - c, k, WHITE);
    ab.drawLine(k, 63 - c, c, 63 - k, WHITE); ab.drawLine(127 - k, 63 - c, 127 - c, 63 - k, WHITE);
  }
  for (uint8_t i = 0; i < c; i++) { ab.drawFastHLine(0, i, c - i, BLACK); ab.drawFastHLine(128 - c + i, i, c - i, BLACK);
    ab.drawFastHLine(0, 63 - i, c - i, BLACK); ab.drawFastHLine(128 - c + i, 63 - i, c - i, BLACK); }
}
void drawHud() {
  // ammo bottom left
  black(8, 54, 36, 8); ab.drawRect(8, 54, 36, 8, WHITE);
  if (bonusT) { if (bonusT > 120 || (bonusT & 8)) tiny(10, 56, "INFINITE"); }
  else for (uint8_t i = 0; i < 6; i++) if (i < ammo) { ab.drawRect(11 + i * 5, 56, 4, 4, WHITE); }
  // score bottom right
  black(77, 54, 43, 8); ab.drawRect(77, 54, 43, 8, WHITE);
  char b[9]; ultoa(score, b, 10); int16_t sx = 118 - strlen(b) * 4; tiny(sx, 56, b);
  // lives as little crosses, left side of the score box (squeezed closer only if the score gets very long)
  uint8_t gap = min(4, (sx - 80) / 5);
  for (uint8_t i = 0; i < lives; i++) { int16_t x = 79 + i * gap; ab.drawFastVLine(x + 1, 55, 6, WHITE); ab.drawFastHLine(x, 57, 3, WHITE); }
}
void drawMsg() {
  if (!msgT || (msgT < 20 && (msgT & 4))) return;
  char b[48]; strcpy_P(b, msg); uint8_t n = strlen(b);
  if (n * 6 <= 118) {                       // fits in the big letters
    black(2, 2, 124, 9); txt(64 - (n * 6 - 1) / 2, 3, b);     // a clean banner across the top
  } else {                                  // tiny letters, split onto two lines if needed
    // too long for one line: show it in two halves, one after the other, so the banner stays one line high
    char* t = b;
    if (n > 29) { for (uint8_t i = n / 2; i < n; i++) if (b[i] == ' ') { b[i] = 0; if (msgT <= MSG_LONG / 2) t = b + i + 1; break; } }
    uint8_t m = strlen(t);
    black(2, 2, 124, 8);                                      // banner stays above everyone's heads
    tiny(64 - (m * 4 - 1) / 2, 3, t);
  }
}
void drawPlay() {
  drawScene();
  for (uint8_t l = 0; l < 3; l++) for (uint8_t i = 0; i < NW; i++) if (wk[i].on && wk[i].lane == l) drawWalker(wk[i]);
  // conversion sparkles / dust
  for (uint8_t i = 0; i < NP; i++) if (pf[i].t) {
    uint8_t r = 17 - pf[i].t;
    for (uint8_t a = 0; a < 8; a++) {
      static const int8_t DX[8] = {2, 1, 0, -1, -2, -1, 0, 1}, DY[8] = {0, 1, 2, 1, 0, -1, -2, -1};
      pset(pf[i].x + DX[a] * r / 2, pf[i].y + DY[a] * r / 3);
    }
  }
  drawHand();
  // crosshair, drawn over the cannon
  { int16_t x = aimX16 >> 4, y = aimY16 >> 4; uint8_t r = 2 + aimLane;
    ab.drawCircle(x, y, r + 1, BLACK); ab.drawCircle(x, y, r - 1, BLACK);   // light halo so it shows over the cannon
    ab.drawCircle(x, y, r, WHITE); ab.drawFastHLine(x - r - 2, y, 2, WHITE); ab.drawFastHLine(x + r + 1, y, 2, WHITE);
    ab.drawFastVLine(x, y - r - 2, 2, WHITE); ab.drawFastVLine(x, y + r + 1, 2, WHITE); }
  drawMsg();        // banner goes under the frame so the frame corners stay crisp
  drawFrame();
  drawHud();
  // bibles fly from the cannon in an arc and shrink with distance
  for (uint8_t i = 0; i < NB; i++) if (bb[i].on) {
    Bible& b = bb[i];
    int16_t p = (int16_t)b.t * 64 / b.dur;            // 0..64
    int16_t sx = 64 * 16 + ((b.tx - 64 * 16) * p >> 6), sy = 56 * 16 + ((b.ty - 56 * 16) * p >> 6);
    int16_t arc = (int16_t)(10 + (2 - b.lane) * 5) * p * (64 - p) / 1024;
    drawBible(sx >> 4, (sy >> 4) - arc, p < 10 ? 0 : p < 26 ? 1 : (p < 46 || b.lane == 2) ? 2 : 3, (b.t >> 2) & 1);
  }
}

// daytime: everything in play is drawn as a night picture, then flipped to black on white
void flip() { for (uint16_t i = 0; i < 1024; i++) ab.sBuffer[i] = ~ab.sBuffer[i]; }

// big letters: the 5x7 font with every dot drawn as an s x s block
void bigTxt(int16_t x, int16_t y, const char* t, uint8_t s) {
  for (; *t; t++, x += 6 * s) {
    uint8_t c = *t; uint8_t g = (c < 32 || c > 90) ? 0 : c - 32;
    for (uint8_t i = 0; i < 5; i++) { uint8_t col = pgm_read_byte(&FONT[g * 5 + i]);
      for (uint8_t r = 0; r < 8; r++) if (col >> r & 1) ab.fillRect(x + i * s, y + r * s, s, s, WHITE); }
  }
}
// SECOND COMING: black screen, the words flash and the screen shakes, then the bonus game starts
#define COMING_LEN 150
void drawComing() {
  int8_t sx = 0, sy = 0;
  if (tick < 60) { uint8_t m = tick < 30 ? 4 : 2; sx = random(-m, m + 1); sy = random(-m, m + 1); }
  bool on = tick >= 100 || ((tick >> 3) & 1) == 0;
  if (on) { bigTxt(10 + sx, 2 + sy, "SECOND", 3); bigTxt(1 + sx, 27 + sy, "COMING!", 3); }
  else ab.fillRect(0, 0, 128, 52, WHITE);       // flash
  txt(25 + sx / 2, 56, "INFINITE AMMO");
}

// ranks for the end screen, by total score
const char R0[] PROGMEM = "PEW WARMER";      const char R1[] PROGMEM = "SUNDAY SCHOOLER";
const char R2[] PROGMEM = "USHER";           const char R3[] PROGMEM = "GIDEON";
const char R4[] PROGMEM = "YOUTH PASTOR";    const char R5[] PROGMEM = "DISCIPLE";
const char R6[] PROGMEM = "MISSIONARY";      const char R7[] PROGMEM = "SAMSON";
const char R8[] PROGMEM = "EVANGELIST";      const char R9[] PROGMEM = "PROPHET";
const char R10[] PROGMEM = "TELEVANGELIST";  const char R11[] PROGMEM = "BILLY GRAHAM";
#define NRANKS 12
const char* const RANKS[NRANKS] PROGMEM = {R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11};
const uint16_t RANK_AT[NRANKS] PROGMEM = {0, 300, 800, 1500, 2200, 3000, 4000, 5000, 6500, 8000, 10000, 13000};

// Bible verse on a black screen between the title and the game, typed out letter by letter
const char V0[] PROGMEM = "\"FOR THE WORD OF GOD"; const char V1[] PROGMEM = "IS QUICK, AND";
const char V2[] PROGMEM = "POWERFUL, AND";       const char V3[] PROGMEM = "SHARPER THAN ANY";
const char V4[] PROGMEM = "TWOEDGED SWORD.\"";
const char* const VERSE[5] PROGMEM = {V0, V1, V2, V3, V4};
#define VERSE_CHARS 78          // letters in the five lines
#define VERSE_DONE (VERSE_CHARS * 2 + 10)
void drawVerse() {
  int16_t left = tick / 2;      // a new letter every 2 frames
  char b[22];
  for (uint8_t i = 0; i < 5 && left > 0; i++) {
    strcpy_P(b, (const char*)pgm_read_word(&VERSE[i]));
    uint8_t n = strlen(b), w = n * 6 - 1;
    if (left < n) b[left] = 0;
    txt(64 - w / 2, 3 + i * 9, b);
    left -= n;
  }
  if (tick >= VERSE_DONE) {
    tiny(64 - (12 * 4 - 1) / 2, 49, "HEBREWS 4:12");
    if ((tick - VERSE_DONE) & 32) tiny(32, 58, "PRESS ANY BUTTON");
  }
}
// controls screen: a little Arduboy with its buttons labelled; the fire button blinks
void drawCtrl() {
  memcpy_P(ab.sBuffer, CTRL, 1024);
  if (tick & 16) ab.fillCircle(CTRL_FIRE_X, CTRL_FIRE_Y, 3, WHITE);
  if (tick & 32) tiny(32, 58, "PRESS ANY BUTTON");
}

// small black Bible for the end screen (rows, bit 0 = left; ink is drawn white, the screen gets flipped)
const uint8_t BOOK8[10] PROGMEM = {0x7F, 0xFF, 0x77, 0xF7, 0x41, 0xF7, 0x77, 0xF7, 0x7F, 0xFE};
void book8(int16_t x, int16_t y, bool flip) {
  for (uint8_t r = 0; r < 10; r++) { uint8_t b = pgm_read_byte(&BOOK8[r]);
    for (uint8_t c = 0; c < 8; c++) if (b >> c & 1) pset(flip ? x + 7 - c : x + c, y + r); }
}
// one line of the results: label, a dotted leader, the number on the right
void statRow(uint8_t y, const char* label, uint32_t v) {
  char b[12]; ultoa(v, b, 10);
  int16_t xe = tiny(12, y, label), xv = 116 - strlen(b) * 4;
  for (int16_t x = xe + 1; x < xv - 2; x += 2) pset(x, y + 4);
  tiny(xv, y, b);
}
void drawOver() {
  book8(20, 0, true); book8(100, 0, false);            // a Bible either side of the heading
  txt(37, 2, "GAME OVER");
  for (uint8_t x = 6; x < 122; x += 2) pset(x, 12);   // dotted rule under the heading
  statRow(15, "CONVERSIONS", totalConv);
  statRow(21, "PARTIAL CONVERSIONS", totalPart);
  statRow(27, "BIBLES BLASTED", totalThrown);
  statRow(33, "SCORE", score);
  statRow(39, "BEST", best);
  char b[20];
  uint8_t r = 0; for (uint8_t i = 1; i < NRANKS; i++) if (score >= pgm_read_word(&RANK_AT[i])) r = i;
  strcpy_P(b, (const char*)pgm_read_word(&RANKS[r]));
  tiny(46, 45, "YOUR RANK");
  int16_t w = strlen(b) * 6 - 1, x = 64 - w / 2;
  txt(x, 51, b);
  // little crosses either side of the rank
  for (int8_t k = -1; k <= 1; k += 2) { int16_t cx = k < 0 ? x - 6 : x + w + 5;
    ab.drawFastVLine(cx, 51, 7, WHITE); ab.drawFastHLine(cx - 2, 53, 5, WHITE); }
  if (tick & 32) tiny(32, 59, "PRESS ANY BUTTON");
}

void setup() {
  ab.begin();
  ab.setFrameRate(60);
  EEPROM.get(EE_BEST, best); best ^= 0xB1B1E000UL; if (best > 9999999UL) best = 0;
}

void loop() {
  if (!ab.nextFrame()) return;
  ab.pollButtons();
  ab.clear();
  tick++;
  switch (mode) {
    case M_TITLE:
      drawTitle();
      if (ab.justPressed(ANY_BTN)) { mode = M_BOOM; tick = 0; ab.setFrameRate(30); drawBoom(); }
      break;
    case M_BOOM:
      drawBoom();
      if (tick >= BOOM_LEN) { mode = M_VERSE; tick = 0; ab.setFrameRate(60); }
      break;
    case M_PLAY:
      updatePlay();
      if (mode == M_PLAY) { drawPlay(); flip(); }
      else if (mode == M_OVER) { drawOver(); flip(); }
      else drawComing();
      break;
    case M_VERSE:
      drawVerse();
      // any button: finish the typing, or once it is all shown, go on to the controls
      if (ab.justPressed(ANY_BTN)) { if (tick < VERSE_DONE) tick = VERSE_DONE; else { mode = M_CTRL; tick = 0; } }
      break;
    case M_CTRL:
      drawCtrl();
      if (tick > 20 && ab.justPressed(ANY_BTN)) { mode = M_PLAY; tick = 0; randomSeed(ab.generateRandomSeed() ^ tick); startGame(); }
      break;
    case M_COMING:
      drawComing();
      if (tick >= COMING_LEN) { mode = M_PLAY; say(M_SECOND); }
      break;
    case M_OVER:
      drawOver(); flip();
      if (tick > 60 && ab.justPressed(ANY_BTN)) { mode = M_TITLE; tick = 0; }
      break;
  }
  ab.display();
}
