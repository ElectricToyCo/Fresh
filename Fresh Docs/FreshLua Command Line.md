FreshLua Command Line

I have an architectural decision to make to bring the console commandline into the executable itself (as opposed to telnet).

In pico-8, the command line functions as a sort of meta-cartridge. It seems to operate within the virtual machine itself, accessing the same drawing system that games do. Not just the command line, of course, but the whole editing system, of which the command line is one mode. In effect, for the editor-enabled executable, a "base", editor game is loaded, which in turn hosts an inner game—"inner" in a peer-like way.

This is as opposed to e.g. the Quake console. Here, the command line is substantially a different _program_ from the game. Naturally they dovetail at the rendering level at least, but the command line is more clearly a peer with ontological siblinghood. Hm, I'm saying more than I yet understand there.

The choice, fundamentally, is whether to implement the command line in lua and thus hosted inside the FantasyConsole class or whether to host it inside Fresh as a peer class to FantasyConsole. I'm drawn to the first solution. It makes less of Fresh—entangles us less with the rest of Fresh. That alone is pretty compelling.

Yeah, that does it.