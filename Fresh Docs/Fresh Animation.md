Fresh Animation
===============

CTCR has put ultimate pressure on the Fresh animation system centered in MovieClip. It is lacking in several ways.

*	CTCR animations affect various parts of the character's animation tree: sometimes just the face, sometimes the eyes and face separately, sometimes the whole body. I need to be able to setup, coordinate, and direct animations from the root with flexibility (things can be done differently in different subtrees) and power (things can be done simply when the desire is simple, such as all-synchronized animation.)
*	Texture animation is handled specially. A MovieClip can animate its own texture (and textureWindow) but nothing else about itself. It cannot, on the other hand, animate descendant textures. The design is lacks cohesion. Right at this very moment I have the need to animate a Sprite's texture window and pivot at the same time. It can't be done.
*	Fresh inherits Flash's homogenous keyframe system. It means that it's easy to setup keyframes, easy to launch an animation, but difficult to:

	*	speed up or slow down animation frame rates
	*	detect the end of an animation

	(The latter problem can be gotten around by putting each animation into its own MovieClip child and then triggering each child when needed.)

What would be better? The fundamental solution is to give root MovieClips direct access to any descendant (not just children). I've already done this, and as far as it goes, it's helpful. But more features are needed and some questions have to be resolved.

*	Feature: DisplayObjectStates need to additionally be able to set the texture and textureWindow for relevant descendants.
*	Question: What do we do with descendants that are not mentioned in a keyframe?
*	Question: What do we do with a descendant that is being animated by more than one parent?
*	Question: Do we get rid of MovieClip texture self-animation?

The feature is no great trouble. It involves dynamic casting, but this isn't problematic in this case. Each DisplayObjectState will be larger in memory by about 20%, but again this isn't too worrisome. Make it so.

Does this mean we get rid of MovieClip self-animation (the last question)? Unless I'm using it extensively already without realizing, yes it does. The simplicity will pay off.

Now what about the second question: what about unmentioned descendants?

This question is amplified by Flash's own policy of hiding any *children* that are not mentioned in a keyframe. This can be a frustrating feature, but it's mired deep in tradition, and admittedly the alternatives aren't immediately appealing. For a button, for example, you generally want several keyframes, each showing a unique child. Being obligated to "turn off" all the unwanted children from each frame clearly adds work in this case.

With descendants it's more complicated, however. The burden of work certainly seems to move the other way: the obligation to "touch" potentially hundreds of minor descendants in order to keep them visible seems clearly arduous.

Do we have two policies then: unmentioned children are "keyframe-hidden" but otherwise descendants are left alone unless explicitly addressed? This does make some sense.

The third question above is related to the second. What happens if a descendant is animated by two separate ancestors?

Clearly, at a minimum, we can do a check and warning if we discover this situation. From a policy standpoint, I would say that the child trumps the ancestor: that is, if root R discovers that descendant P is animating deeper-descendant C, it logs a warning and relinquishes control of C. If you don't want that behavior, write code to stop P's animation.

The work, then, is:

*	DisplayObjectState gains texture, textureWindow, and isTextureSet members and applies them when able.
*	Keyframes lose their corresponding data and features. This changes:
	*	SimpleButtons
	*	Characters (of course)
*	As a developer debug measure, a MovieClip seeking a descendant confirms that no intervening children also address the descendant and reports a warning if they do.

