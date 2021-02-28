FreshLua - Run modes

I'm trying to figure out how I'm going to make FreshLua run in its various modes. It's confusing.

Here are the possible "execution cases".

1. Create, build, and run a specific game from within Xcode.
2. Run FreshLua in editor mode a standalone app; build games in "user mode" within the Document directory.
3. Run FreshLua in editor mode as a standalone web app
4. Run FreshLua with a specific game as a webapp.
5. FreshLua "editor" creates specific game as a platform-specific app.

Pico-8 acts as #2, 4, and 5.

Tic-80 adds #3 and, I suppose, #1.

The basic components of this breakdown are:

1. Are you building and running from Xcode or running FreshLua as an already-built app?
2. Is FreshLua running in editor mode?
3. Does FreshLua have a game bundled with it (other than the editor) that it loads automatically?

These are orthogonal: thus there are eight options.

In terms of implementation, it really all comes down to the assets directory and the user's Documents directory.

If the assets directory contains the editor game, it *can* be loaded. But should it?

If it contains an autorun game, it can and probably should be loaded.

Either way, the user's Documents directory games are available.

So: when should the autorun game be loaded? Probably whenever it exists.