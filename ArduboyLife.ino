/*
Copyright (c) 2015 Scott Allen

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
*/

// -------------------------------------
// Conway's Game Of Life for the Arduboy
// -------------------------------------

#include <SPI.h>
#include <EEPROM.h>
#include <Arduboy.h>

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

boolean soundOn = 0;
unsigned int lastTone = 0;

long randSeed = 42; // The answer to life, the universe and everything...

Arduboy aboy;

void setup() {
  aboy.start();
  aboy.clearDisplay();
  grid = (uint8_t (*)[LIFEWIDTH]) aboy.getBuffer();

  // Display the intro screen
  aboy.setTextSize(2);
  aboy.setCursor(23, 2);
  aboy.print("ARDUBOY");
  aboy.setTextSize(1);
  aboy.setCursor(40, 23);
  aboy.print("CONWAY'S");
  aboy.setCursor(43, 35);
  aboy.print("Game of");
  aboy.setTextSize(2);
  aboy.setCursor(41, 48);
  aboy.print("LIFE");
  aboy.setTextSize(1);

  aboy.display();
  delayPoll(3000);

  // Display the button help screen.
  // This will be the initial grid pattern unless
  // the NEW or REPLAY button is pressed.
  // Note that the REPLAY button won't restart this pattern.
  showHelp();
  liveCells = countCells();
  delayPoll(8000);

  aboy.initRandomSeed();
}

void loop() {
  switch (aboy.getInput()) {
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
  waitRelease(button);
  delayPoll(1000);
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
    soundOn = !soundOn;
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
    soundOn = !soundOn;
    waitRelease(BTN_FASTER + BTN_SLOWER);
  }
}

// Generate a new game grid.
// Places a random number of random characters
// at random positions on the screen.
void genGrid(long seed) {
  int numChars;

  aboy.clearDisplay();
  randomSeed(seed);
  numChars = random(10, 100);
  for (int c = 0; c <= numChars; c++) {
    aboy.drawChar(random(LIFEWIDTH - 1 - 6), random(LIFEHEIGHT - 1 - 8),
                  random(1, 255), 1, 0, 1);
  }
  aboy.display();
  stepCount = 0;
  liveCells = countCells();
  if (soundOn) {
    lastTone = 0;
    playTone();
  }
}

// Iterate and display the next generation
void nextStep() {
  liveCells = lifeIterate(grid);
  aboy.display();
  stepCount++;
  if (soundOn) {
    playTone();
  }
}

// Display the button help screen
void showHelp() {
  aboy.clearDisplay();

  aboy.drawChar(47, 11, 0x1f, 1, 0, 1);
  aboy.drawChar(43, 15, 0x10, 1, 0, 1);
  aboy.drawChar(47, 19, 0x1e, 1, 0, 1);
  aboy.drawChar(51, 15, 0x11, 1, 0, 1);

  aboy.setCursor(32, 3);
  aboy.print("FASTER");
  aboy.setCursor(6, 15);
  aboy.print("REPLAY");
  aboy.setCursor(58, 15);
  aboy.print("NEW");
  aboy.setCursor(32, 27);
  aboy.print("SLOWER");

  aboy.drawFastHLine(69, 6, 19, 1);
  aboy.drawFastHLine(69, 30, 19, 1);
  aboy.drawFastVLine(88, 6, 25, 1);
  aboy.drawFastHLine(89, 18, 5, 1);

  aboy.setCursor(96, 15);
  aboy.print("SOUND");

  aboy.setCursor(92, 42);
  aboy.print("RUN");
  aboy.setCursor(28, 53);
  aboy.print("PAUSE / STEP");

  aboy.fillCircle(115, 45, 4, 1);
  aboy.fillCircle(105, 56, 4, 1);

  aboy.display();
}

// Display the game status.
// Leave the screen buffer unaltered.
void showInfo() {
  uint8_t saveBuf[LIFEWIDTH * 3]; // screen buffer save area

  memcpy(saveBuf, grid, sizeof saveBuf);
  memset(grid, 0,  sizeof saveBuf);

  aboy.setCursor(0, 0);
  aboy.print("Speed ");
  aboy.print(speedNum);
  aboy.setCursor(72, 0);
  aboy.print("Sound ");
  if (soundOn) {
    aboy.println("ON");
  }
  else {
    aboy.println("OFF");
  }
  aboy.print("Cells ");
  aboy.print(liveCells);
  aboy.setCursor(84, 8);
  if (paused) {
    aboy.println("PAUSED");
  }
  else {
    aboy.println("RUNNING");
  }
  aboy.print("Step  ");
  aboy.print(stepCount);

  aboy.display();
  memcpy(grid, saveBuf, sizeof saveBuf);
}

// Return a count of the number of live cells on the current grid
unsigned int countCells() {
  unsigned int total = 0;
  unsigned char *c = (unsigned char *) grid;

  for (unsigned int i = 0; i < (LIFEWIDTH * LIFEHEIGHT / 8); i++, c++) {
    for (uint8_t j = 0; j < 8; j++) {
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
    aboy.tunes.tone(newTone, 2100);
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
  while (aboy.not_pressed(button) == false) {
    if (((button == BTN_SLOWER) || (button == BTN_FASTER)) &&
         aboy.pressed(BTN_SLOWER + BTN_FASTER)) {
      return false;
    }
  }
  delay(debounceWait);
  aboy.display();
  return true;
}

// Delay for the specified number of miliseconds or
// until a button is pressed
void delayPoll(unsigned long msDelay) {
  unsigned long waitTime = millis() + msDelay;

  while (((long) (millis() - waitTime) < 0) && (aboy.getInput() == 0)) {}
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

// number of bits in values from 0 to 7
static const unsigned char bitCount[] = { 0, 1, 1, 2, 1, 2, 2, 3 };

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
    left = (((unsigned int) (cur[rowA][LIFEHIGHCOL])) >> 7) |
           (((unsigned int) cur[row][LIFEHIGHCOL]) << 1) |
           (((unsigned int) cur[rowB][LIFEHIGHCOL]) << 9);

    centre = (((unsigned int) (cur[rowA][0])) >> 7) |
             (((unsigned int) cur[row][0]) << 1) |
             (((unsigned int) cur[rowB][0]) << 9);

    colR = 1;
    for (col = 0; col < LIFEWIDTH; col++) {
      right = (((unsigned int) (cur[rowA][colR])) >> 7) |
              (((unsigned int) cur[row][colR]) << 1) |
              (((unsigned int) cur[rowB][colR]) << 9);

      grid[row][col] = lifeByte(left, centre, right);

      left = centre;
      centre = right;

      colR = (colR < LIFEHIGHCOL) ? colR + 1 : 0;
    }
    rowA = (rowA < LIFEHIGHROW) ? rowA + 1 : 0;
    rowB = (rowB < LIFEHIGHROW) ? rowB + 1 : 0;
  }
  return lifeCellCount;
}

// Calculate the next generation for 8 vertical cells (one byte of the
// array) that have been packet along with their neighbours into ints.
uint8_t lifeByte(unsigned int left, unsigned int centre, unsigned int right) {
  unsigned char count;
  uint8_t newByte = 0;

  for (unsigned char i = 0; i < 8; i++) {
    count = bitCount[left & 7] + bitCount[centre & 7] + bitCount[right & 7];

    if ((count == 3) || ((count == 4) && (centre & 2))) {
      newByte |= (1 << i);
      lifeCellCount++;
    }
    left >>= 1;
    centre >>= 1;
    right >>= 1;
  }
  return newByte;
}

