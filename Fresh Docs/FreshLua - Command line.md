FreshLua - Command line

I'm implementing the command line within FreshLua. My wish is to make it a lot like pico-8's, though this is not strictly necessary. The tricky bit has to do with clearing, and indeed with the broader question of drawing.

In pico-8, the cls() function indeed clears the screen as well as setting the print y value to draw to the top of the screen. If cls() is not called, each subsequent cls() function adds another line to the bottom of the series, moving downward in y, but when it hits the bottom of the screen, all the lines are shifted upward. In other words, it's as if each print method adds a line to an infinite list, and the camera into the list is always clamped to include the bottom of the list. There are actually two histories: the print history and the command history.

I see now. If the print function has no x or y value, it shifts the rest of the graphics upward by one "line", then prints at the default location. This location is clamped to the bottom line of the screen.

Other details:

-	The print function takes on whatever color the last draw command had.

-	If the game has no _draw() function, the game acts like a commandline app, stopping as soon as it has run.

The trick, then, is how to do clearing the way pico-8 does.

Probably I want a render target.

