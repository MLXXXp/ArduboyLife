// -------------------------------------
// Conway's Game Of Life for the Arduboy
// -------------------------------------

/*
Copyright (c) 2017 Scott Allen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

Modified by Bill Welch so cell size can be changed from one pixel (1x1) to four pixels (2x2). Press the RUN button
while holding the NEW button to toggle the size.
*/

// Cells are 2x2 or 1x1; just look at top left corner to see if live or dead.

#include <Arduboy2.h>
#include <ArduboyTones.h>

// Width and height of the screen in pixels (one pixel per cell)
#define LIFEWIDTH WIDTH
#define LIFEHEIGHT HEIGHT // must be a multiple of 8

// Button mappings
#define BTN_NEW RIGHT_BUTTON
#define BTN_REPLAY LEFT_BUTTON
#define BTN_SLOWER DOWN_BUTTON
#define BTN_FASTER UP_BUTTON
#define BTN_PAUSE A_BUTTON
#define BTN_RUN B_BUTTON

const int debounceWait = 50; // delay for button debounce

// Pointer to the screen buffer as a two dimensional array,
// the way the Life engine wants it.
static uint8_t (*grid)[LIFEWIDTH];

unsigned long stepCount = 0;
boolean paused = false;

unsigned int liveCells;

unsigned int speedDly = 64;
unsigned char speedNum = 6;

unsigned int lastTone = 0;

long randSeed = 42; // The answer to life, the universe and everything...

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);

unsigned int lifeIterate(uint8_t grid[][LIFEWIDTH]);


// 1x1 cells .... number of bits in values from 0 to 7 using mask 0x07.
static const unsigned char bitCount1x1[] = { 0, 1, 1, 2, 1, 2, 2, 3 };

// 2x2 cells .... number of bits in values 0x00 to 0x15 for every other bit using mask 0x15.
// The top left corner of the 2x2 cell is tested for life. Newly generated live cells have all four pixels on.
//                                          00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 
//                                           0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 
static const unsigned char bitCount2x2[] = { 0, 1, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 2, 3 };

static unsigned char bitCount[22];

int cellSize;  // 1 : cells are 1x1 pixels; 2 :2x2 pixels. See setupCellSize(newSize);

void setupCellSize(int newSize){
  cellSize = newSize;
  if (cellSize==1) {
    memcpy (bitCount,bitCount1x1,sizeof bitCount1x1);
  } else {
    memcpy (bitCount,bitCount2x2,sizeof bitCount2x2);
  }
}  

void setup() {

  setupCellSize(1); // Startup using a cell size of 1x1. Pressing RUN while the NEW button is pressed will toggle cell size
                    // between 1x1 and 2x2.
  arduboy.begin();
  arduboy.audio.off();
  arduboy.clear();

  grid = (uint8_t (*)[LIFEWIDTH]) arduboy.getBuffer();

  // Display the intro screen
  arduboy.setTextSize(2);
  arduboy.setCursor(23, 2);
  arduboy.print(F("ARDUBOY"));
  arduboy.setTextSize(1);
  arduboy.setCursor(40, 23);
  arduboy.print(F("CONWAY'S"));
  arduboy.setCursor(43, 35);
  arduboy.print(F("Game of"));
  arduboy.setTextSize(2);
  arduboy.setCursor(41, 48);
  arduboy.print(F("LIFE"));
  arduboy.setTextSize(1);

  arduboy.display();
  delayPoll(3000);

  // Display the button help screen.
  // This will be the initial grid pattern unless
  // the NEW or REPLAY button is pressed.
  // Note that the REPLAY button won't restart this pattern.
  showHelp();
  liveCells = countCells();
  delayPoll(8000);

  arduboy.initRandomSeed();
}

void loop() {
  switch (arduboy.buttonsState()) {
    case BTN_PAUSE:
      pauseGame();
      break;

    case BTN_RUN:
      runGame();
      break;

    case BTN_NEW:
      randSeed = random(LONG_MIN, LONG_MAX) + millis();
      newGame(BTN_NEW);
      break;

    case BTN_REPLAY:
      newGame(BTN_REPLAY);
      break;

    case BTN_SLOWER:
      goSlower();
      break;

    case BTN_FASTER:
      goFaster();
      break;
  }

  if (!paused) {
    nextStep();
  }

  if (speedDly != 0) {
    delayPoll(speedDly);
  }
}

// Handler for NEW and REPLAY game buttons
void newGame(uint8_t button) {
  genGrid(randSeed);
 if ( waitRelease(button) == false){
 //  Run button pressed with the NEW button - toggle 1x1 or 2x2 cell sizes.
   setupCellSize( (cellSize==1) ? 2 : 1 );  // alternate cellSize between 1 and 2
 }
     waitRelease(BTN_NEW + BTN_RUN);
//  delayPoll(1000);
}

// Handler for PAUSE / STEP button.
// Will step if paused.
void pauseGame() {
  if (paused) {
    nextStep();
  }
  else {
    paused = true;
  }
  waitRelease(BTN_PAUSE);
}

// Handler for RUN button
void runGame() {
  paused = false;
  waitRelease(BTN_RUN);
}

// Handler for the DECREASE SPEED button.
// Decrements the speed number and doubles the current speed limiting delay,
// up to a maximum of about 2 seconds.
// If both speed buttons are pressed, toggle sound and
// leave the speed unchanged.
void goSlower() {
  unsigned int lastspeedDly = speedDly;
  unsigned char lastSpeedNum = speedNum;

  if (speedDly < 16) {
    speedDly = 16;
    speedNum = 8;
  }
  else if (speedDly <= 1024) {
    speedDly *= 2;
    speedNum--;
  }

  if (waitRelease(BTN_SLOWER) == false) {
    speedNum = lastSpeedNum;
    speedDly = lastspeedDly;
    arduboy.audio.toggle();
    waitRelease(BTN_SLOWER + BTN_FASTER);
  }
}

// Handler for the INCREASE SPEED button.
// Increments the speed number and halves the current speed limiting delay,
// down to the minimum possible (no extra delay).
// If both speed buttons are pressed, toggle sound and
// leave the speed unchanged.
void goFaster() {
  unsigned int lastspeedDly = speedDly;
  unsigned char lastSpeedNum = speedNum;

  if (speedDly == 16) {
    speedDly = 0;
    speedNum = 9;
  }
  else if (speedDly > 0) {
    speedDly /= 2;
    speedNum++;
  }

  if (waitRelease(BTN_FASTER) == false) {
    speedNum = lastSpeedNum;
    speedDly = lastspeedDly;
    arduboy.audio.toggle();
    waitRelease(BTN_FASTER + BTN_SLOWER);
  }
}

void genGrid(long seed) {
  int numChars;

  arduboy.clear();
  randomSeed(seed);
  numChars = random(10, 100);
  for (int c = 0; c <= numChars; c++) {
    arduboy.drawChar(random(LIFEWIDTH - 1 - 6), random(LIFEHEIGHT - 1 - 8),
                  random(1, 255), 1, 0, 1);
  }
  arduboy.display();
  stepCount = 0;
  liveCells = countCells();
  if (arduboy.audio.enabled()) {
    lastTone = 0;
    playTone();
  }
}

// Iterate and display the next generation
void nextStep() {
  liveCells = lifeIterate(grid);
  arduboy.display();
  stepCount++;
  if (arduboy.audio.enabled()) {
    playTone();
  }
}

// Display the button help screen
void showHelp() {
  arduboy.clear();

  arduboy.drawChar(47, 11, 0x1f, 1, 0, 1);
  arduboy.drawChar(43, 15, 0x10, 1, 0, 1);
  arduboy.drawChar(47, 19, 0x1e, 1, 0, 1);
  arduboy.drawChar(51, 15, 0x11, 1, 0, 1);

  arduboy.setCursor(32, 3);
  arduboy.print(F("FASTER"));
  arduboy.setCursor(6, 15);
  arduboy.print(F("REPLAY"));
  arduboy.setCursor(58, 15);
  arduboy.print(F("NEW"));
  arduboy.setCursor(32, 27);
  arduboy.print(F("SLOWER"));

  arduboy.drawFastHLine(69, 6, 19, 1);
  arduboy.drawFastHLine(69, 30, 19, 1);
  arduboy.drawFastVLine(88, 6, 25, 1);
  arduboy.drawFastHLine(89, 18, 5, 1);

  arduboy.setCursor(96, 15);
  arduboy.print(F("SOUND"));

  arduboy.setCursor(92, 42);
  arduboy.print(F("RUN"));
  arduboy.setCursor(28, 53);
  arduboy.print(F("PAUSE / STEP"));

  arduboy.fillCircle(115, 45, 4, 1);
  arduboy.fillCircle(105, 56, 4, 1);

  arduboy.display();
}

// Display the game status.
// Leave the screen buffer unaltered.
void showInfo() {
  uint8_t saveBuf[LIFEWIDTH * 3]; // screen buffer save area only top 3 lines (24 pixels) used

  memcpy(saveBuf, grid, sizeof saveBuf);
  memset(grid, 0, sizeof saveBuf);

  arduboy.setCursor(0, 0);
  arduboy.print(F("Speed "));
  arduboy.print(speedNum);
  arduboy.setCursor(72, 0);
  arduboy.print(F("Sound "));
  if (arduboy.audio.enabled()) {
    arduboy.println(F("ON"));
  }
  else {
    arduboy.println(F("OFF"));
  }
  arduboy.print(F("Cells "));
  arduboy.print(liveCells);
  arduboy.setCursor(84, 8);
  if (paused) {
    arduboy.println(F("PAUSED"));
  }
  else {
    arduboy.println(F("RUNNING"));
  }
  arduboy.print(F("Step  "));
  arduboy.print(stepCount);

  arduboy.display();
  memcpy(grid, saveBuf, sizeof saveBuf);
}

// Return a count of the number of live cells on the current grid.
// If cells are 2x2, just look at top left corner to see if live or dead.
unsigned int countCells() {
  unsigned int total = 0;
  unsigned char *c = (unsigned char *) grid;

// !!!!!! if 2x2, then need to skip odd columns as well as odd bits in each byte.

  for (unsigned int i = 0; i < (LIFEWIDTH * LIFEHEIGHT / 8); i+=1, c+=1) {
    for (uint8_t j = 0; j < 8; j+=cellSize) {
      if ((*c & (1 << j)) != 0) {
        total++;
      }
    }
  }
  return total;
}

// Play a tone with a frequency based on the number of live cells.
// The more cells, the higher the pitch.
void playTone() {
  unsigned int newTone;

  if ((newTone = (liveCells * 3 + 20)) != lastTone) {
    sound.tone(newTone, 2100);
    lastTone = newTone;
  }
}

// Show status information then wait for button release,
// including a simple delay based debounce.
//
// If, while waiting for the Slower or Faster button,
// the other speed button is also pressed, return false.
// In this case another call to this function is required
// to wait for both buttons to be released.
// Otherwise, return true.
boolean waitRelease(uint8_t button) {
  showInfo();
  delay(debounceWait);
  while (arduboy.notPressed(button) == false) {
    if (((button == BTN_SLOWER) || (button == BTN_FASTER)) &&
         arduboy.pressed(BTN_SLOWER + BTN_FASTER)) {
      return false;
    }
    if ((button == BTN_NEW) && arduboy.pressed(BTN_NEW + BTN_RUN)) {
      return false;
    }
  }
  delay(debounceWait);
  arduboy.display();
  return true;
}

// Delay for the specified number of miliseconds or
// until a button is pressed
void delayPoll(unsigned long msDelay) {
  unsigned long waitTime = millis() + msDelay;

  while (((long) (millis() - waitTime) < 0) && (arduboy.buttonsState() == 0)) {}
}

//------------------------------------------------------------
// The Life engine
//
// Updates the provided grid with the next generation.
//
// Returns the number of live cells in the new grid.
//
// Only the torus type finite grid is implemented. (Left edge joins to the
// right edge and top edge joins to the bottom edge.)
//
// The grid is mapped as horizontal rows of bytes where each byte is a
// vertical line of cells with the least significant bit being the top cell.

// LIFEWIDTH and LIFEHEIGHT must have been previously defined.
// LIFEWIDTH is the width of the grid.
// LIFEHEIGHT is the height of the grid and must be a multiple of 8.
#define LIFELINES (LIFEHEIGHT / 8)
#define LIFEHIGHROW (LIFELINES - 1)
#define LIFEHIGHCOL (LIFEWIDTH - 1)

static unsigned int lifeCellCount; // Total number of live cells

  unsigned int lifeIterate(uint8_t grid[][LIFEWIDTH]) {
  uint8_t cur[LIFELINES][LIFEWIDTH]; // working copy of the current grid

  unsigned char row, col; // current row & column numbers
  unsigned char rowA, rowB, colR; // row above, row below, column to the right
  unsigned int left, centre, right; // packed vertical cell groups
  
  memcpy(cur, grid, sizeof cur);

  lifeCellCount = 0;

  rowA = LIFEHIGHROW;
  rowB = 1;
  for (row = 0; row < LIFELINES ; row++) {
    left = (((unsigned int) (cur[rowA][LIFEHIGHCOL-1])) >> (8-cellSize)) |
           (((unsigned int) cur[row][LIFEHIGHCOL-1]) << cellSize) |
           (((unsigned int) cur[rowB][LIFEHIGHCOL-1]) << (8+cellSize));

    centre = (((unsigned int) (cur[rowA][0])) >> (8-cellSize)) |
             (((unsigned int) cur[row][0]) << cellSize) |
             (((unsigned int) cur[rowB][0]) << (8+cellSize));

    colR = cellSize;
    for (col = 0; col < LIFEWIDTH-1; col+=cellSize) {
      right = (((unsigned int) (cur[rowA][colR])) >> (8-cellSize)) |
              (((unsigned int) cur[row][colR]) << cellSize) |
              (((unsigned int) cur[rowB][colR]) << (8+cellSize));
   
      grid[row][col] = lifeByte(left, centre, right);
      if (cellSize==2) {grid[row][col+1] = grid[row][col];}
      left = centre;
      centre = right;

      colR = (colR < LIFEHIGHCOL-1) ? colR + cellSize : 0;
    }
    rowA = (rowA < LIFEHIGHROW) ? rowA + 1 : 0;
    rowB = (rowB < LIFEHIGHROW) ? rowB + 1 : 0;
  }
  return lifeCellCount;
}

// Calculate the next generation for 4 or 8 vertical cells (one byte of the
// array) that have been packed along with their neighbours into ints.

  uint8_t lifeByte(unsigned int left, unsigned int centre, unsigned int right) {
  unsigned char count;
  uint8_t newByte = 0;
  uint8_t cellMask;
  uint8_t mask;
  uint8_t bits;
    if (cellSize==1) {
      mask=0x7;
      bits=1;
      cellMask=2;
    } else {
      mask=0x15;
      bits=3;
      cellMask=4;
    }
    for (unsigned char i = 0; i < 8; i+=cellSize) {
      count = bitCount[left & mask] + bitCount[centre & mask] + bitCount[right & mask];
      if ((count == 3) || ((count == 4) && (centre & cellMask))) {
        newByte |= (bits << i);
        lifeCellCount++;
      }
      left >>= cellSize;
      centre >>= cellSize;
      right >>= cellSize;
    }
  return newByte;
}
