# Conway's Game of Life for the Arduboy

By Scott Allen

## Description

An implementation of Conway's Game of Life, a cellular automaton, for the [Arduboy](https://www.arduboy.com/), a tiny Arduino compatible game system.

[Conway's Life on Wikipedia](https://en.wikipedia.org/wiki/Conway's_Game_of_Life)

Each pixel is a cell. Only the torus type finite grid is implemented (The left edge joins to the right edge and the top edge joins to the bottom edge).

[Video on YouTube](https://youtu.be/tmZ1W80oE4M)

### NOTE

This sketch double buffers the display image and therefore uses a large amount of RAM. It’s likely to be affected by a bootloader conflict problem, causing issues when trying to upload a new sketch after ArduboyLife has been loaded.

Try holding down the UP button when powering up and plugging in your Arduboy. This will enter *flashlight* mode. The RGB LED and display will light up white. (If you have one of the later Kickstarter Arduboys, with the RGB LED installed wrong, then the RGB LED will not turn on but if the display goes white then you’re probably in *flashlight* mode.) You should then be able to load a new sketch.

## Usage

Buttons used to control the game are as follows:

Button    | Function
------    | --------
A         | Pause. If already paused, step once to the next generation
B         | Run at the current speed. Pause while held, to view info
Right     | Start a new game
Left      | Restart with the same initial pattern
Up        | Increase the speed
Down      | Decrease the speed
Up + Down | Toggle sound on and off. Speed is left unchanged

When first started, an introduction screen appears for a few seconds, then a help screen showing the functions of the buttons is displayed. After another delay, the game will run with the help screen used as the initial grid pattern. The intro and help screens can be bypassed by pushing any button. If the *New* button is pressed, a random pattern grid will be generated. The *Replay* button will generate a fixed pattern that is different from the help screen.

In addition to performing their assigned functions, all buttons will cause status information to be displayed while held. This includes the speed, number of live cells, the step (generation) number, whether the game is running or paused, and whether sound is on or off.

### Sound

If sound is enabled, a tone will be generated that is proportional to the number of live cells currently on the grid. The more cells exist, the higher the frequency of the tone. Each tone is generated for a fixed duration of a few seconds or until a different tone starts. If a new cell count equals the previous one, the tone is not restarted. Therefore, sound will time out and stop if the game gets to a state where the number of cells doesn't change from one step to the next. A tone will also time out when the game is paused or if a button is held down long enough.

## Required Libraries

[Arduboy2](https://github.com/MLXXXp/Arduboy2)

## License

Copyright (c) 2015-2019 Scott Allen

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

