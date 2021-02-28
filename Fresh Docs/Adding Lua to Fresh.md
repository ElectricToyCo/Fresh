Adding Lua to Fresh

I've developed a basic implementation of Lua within Fresh, but it's time to think about how it should really work.

# Goals

1. Enable instant-reload of game code.
2. Enable use of coroutines.
3. Simplify game code syntax generally; Lua is just briefer than C++.
4. Retain the value of Fresh (pathfinding, screen effects, physics, etc.).
5. Retain the persistence policy of Fresh (everything "just reloads", without undue concern for versioning).

# Approaches

There are several different approaches to integrating the two things, and they're _not_ mutually exclusive.

1. Create a ScriptedDisplayObject class that functions, all alone, something like a pico-8 simulator. It alone offers Lua scripting, and it offers that script an API. You can inject a ScriptedDisplayObject into the Fresh scene graph wherever you like (as with other DisplayObjects), but it updates and renders using Lua.
2. Run script at the Stage (or application) level. The script doesn't pertain to the host directly, but instead makes generalized calls to manipulate the Stage scene graph.
3. Any Object can have ScriptedFunction members that are implemented via Lua scripts. In essence, any DisplayObject's update(), render(), and perhaps touchBegin() (and similar) calls may be forwarded to Lua scripts that manipulate the object.
4. The Fresh object system is mirrored completely in Lua, such that you can even extend C++ objects (MovieClip, for example) into a Lua subclass. This is analogous to UnrealScript.

It seems to me that these approaches deal with a set of questions.

* Where does the script "live"? Who owns it? Who loads the actual script file or files? Who calls them?
* How does the script interact with Fresh? Primarily through scenegraph manipulation, or primarily through a pico-8-like API?

Keep in mind that persistence is the dominant problem, no matter which way you slice it. Lua will not, by default, want to persist. None of these problems avoid the need to ensure that it does persist.

The simplest solution is #1. You can make it entirely pico-8-like. Reload is a simple command via console. Create whatever API you like and let the script go to town; but whatever you do, it lives entirely within individual DisplayObject instances (almost always just one). You get all the benefits of Fresh in terms of rendering and even code: arbitrary sprite textures, for example. No persistence to start with, but adding it later, though not easy, disrupts nothing else in the whole system. The disadvantage to this solution is twofold (though one of the folds is nonsense): (1) limitation of the API, and (2) regret over sunk cost. Limitation meaning that everything the Lua can do, it must be given the power to do through explicit construction of the API. (Incidentally, you really want to make sure, in this strategy, that the Lua code has no direct access to Fresh otherwise. Simplicity and isolation of concern are major benefits.) The regret disadvantage looms large emotionally but is not compelling logically: it's sad that you've got this massive scenegraph sprite-based rendering system, and you ignore 99% of it. Effectively it's a new engine, in fact. And indeed, eventually I'm likely to remove a huge mass of the code and essentially build a new engine (probably based on SDL).

This solution makes so much sense that I can't get excited about the others. They're more complex, more invasive, but with not a huge advantage, and with real temptation to complicated development where we build C++ classes and .fresh files, then "fill them out" or manipulate them with Lua. No.

It's #1 all the way.


---------------------

We've reached the next question.

The FantasyConsole needs to present a rendering API (among other things) to the Lua code. How should this rendering be structured. Goals:

- Immediate mode rendering API a la pico-8
- All the graphical fidelity (high resolution textures; bilinear filtering) of Fresh, however.
- Spritesheet not unwelcome

The fundamental question is easy to identify: should rendering calls fundamentally amend a texture, later rendered, or should they front "real" (i.e. OpenGL) rendering calls?

I'm thinking the latter. That is, the fantasy console is more like a browser than it is like pico-8. The "screen" is a logical space, not a grid of pixels. Fractional coordinates are perfectly legitimate in every way: they're even antialiased where possible. Shaders, even, may be understood and manipulated by the API: you're not merely doctoring pixel colors. The API then, is OpenGL-like, not pico-8 like as such.

I'll let the class itself specify not only the spritesheet but it's fundamental dimensions.

We get into potential trouble around functions like `cls()`, but I don't think that's fatal.

One struggle I'm going to have is whether to implement the immediate mode functions with retained mode-like objects; e.g., implement a `print()` call by creating a Fresh `TextField` as a child and allowing it to exist for one (draw) frame before dying. This is the sunk cost worry again (but more rational here): one hates to reimplement capabilities like text rendering from scratch, and yet implementing it through the rapid creation and deletion of objects is expensive and... not a clean way to do it if you were starting from scratch, for sure.

---------------------------

Okay, next question: where to actually put the API implementations?

The problem is simple: the API is large and basically modular, so deserves to be in a different contexts. Yet, it should be unified by an object that coordinates it.

Options:

1. Make all API functions members of a single class.
2. Make groups of API functions live inside distinct classes, but attach those instances to the central instance.
3. Make all API functions "loose" C functions, but pass the central instance to them as a required parameter.

The advantage of #1 is significant: centralizing all API calls simplifies the class structure and avoids excessive call forwarding/redirection. The downside is code bloat. How might I separate the API?

I think I'll treat the different parts of the API as distinct header/C++ file pairs, where the header is intended to be embedded _inside_ the class itself. Yes. The header, then, can also add private data sections.

-------------------

Next issue: collecting the "script" and its constituent parts (textures, audio files, shaders) into some kind of bundle or set, and later packaging them into a standalone game.

This issue is best seen in terms of its touch points with reality.

In engine development, I run 
the engine app from Xcode. I use the console (currently telnet) to load a script file (and friends). These most naturally live in the app's sandboxed document area.

In game development, I run the engine app on its own. I use the console to load a script file (and friends). These most naturally live in the app's sandboxed document area.

For game deployment, there are a few options, but the basic principle is to bundle up the game's files along with the app itself while building the app bundle (via Emscripten, Xcode, or as an operation of the engine app itself), and force the app to start with that "cartridge" (and maybe even only that cartridge).

There are a few problems here, but the essential or common one is that of bundling up the game "cartridge." What are the goals?

- collects multiple files (for audio at least)
- tightly coupled
- directly editable by external editors (e.g. Photoshop, GarageBand export, Sublime Text)
- platform independent

A simple approach is just to point to a base folder. All desired resources either have fixed paths within that folder (like "/main.lua" or "/spritesheet.png") or may be freely referenced from code relative to the folder (like "/bark.mp3" or "/music/title.mp3"). This folder can sit absolutely anywhere, but by default (i.e. when using the `load` command) it sits within the app's base path. It has a unique name. To "load" a cartridge is actually to load that base folder.

This solution is weak on coupling but otherwise strong. When shipping a "real" game, you simply bundle the folder up with the rest of the app package and load it. Indeed, it might be the default behavior of the engine that if it sees a child folder called "autorun" or something, it loads that cartridge immediately.

I don't think, by the way, that "cartridge" is that right term. "game" is better.