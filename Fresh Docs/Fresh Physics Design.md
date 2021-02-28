** Fresh Physics Design **


The Goals
=========

1.	Make at least one 2D physics provider (e.g. Chipmunk, Box2D) available for "driving" Fresh DisplayObjects.
2.	Make physics systems more convenient to use (e.g. providing automatic splitting of concave collision polygons.)
3.	Support powerful authoring of physics scenes, mechanisms, contraptions, etc.
4.	Support threaded physics independent from rendering.
5.	Avoid excessive "wrapping" of provider functionality--i.e. that might require a lot of maintenance.
6.	Remain provider-agnostic.

The Problems
============

I need physics-generating DisplayObjects (currently PhysicsBody and PhysicsPin) to support editing of the physical scene. And yet this approach locks physical objects into a particular inheritance hierarchy. Not the end of the world, but a "has-a" approach would allow more flexibility--e.g. attaching physics objects to Sprites in some cases and ParticleEmitters in other cases. But that solution just creates too much redundancy: one DisplayObject type per physics object type to support authoring (and these objects instantly die in the game), and then a "linkage" object to link physics objects with DisplayObjects. So I'm going with an inheritance based approach, and it's working okay.

Memory management is the big problem now, and it's confusing. There's a policy problem.

There is a set of coherent policies available:

1.	**Laissez-Faire:** Physics objects are created and destroyed on an ad-hoc basis. This corresponds to the use of new and delete (actually cp*Create() and cp*Destroy()) everywhere.
2.	**Pure ownership:** Physics objects are owned by their Fresh-side proxies. This corresponds to the use of std::unique_ptr< cpBody > from PhysicsBodies and such. When the PhysicsBody dies, the physics object dies.
3.	**Pure ownership with detachment:** Physics objects are owned by their Fresh-side proxies, but the relationship isn't locked: PhysicsBody can die prior to the object it owns, giving up ownership to the system. Like std::unique_ptr<> with std::move().
4.	**Shared ownership:** Important though this concept is in C++, it's not clear to me that it's helpful in this case. The idea is that objects reference physics objects via shared_ptr<>, which kills the object when nothing else is using it. I'm just not convinced this is conceptually distinct from Case #2 or #3.

A complication is that single Fresh objects often need to maintain multiple physics objects; for example, this is very common with constraints. Sometimes the constraint is "unmanaged" (we're happy for it to be cleaned up by the physics system when it cleans everything up); other times, the constraint must be "managed"--cleaned up by the owner.

A further complication is that objects have difficult ordering physics-creation ordering needs. See PhysicsBody::createPhysics() for a suggestion of the complexities involved: sometimes children must be made physical before parents; sometimes the children are destroyed before being made physical; sometimes they're made physical after the parent; sometimes the order doesn't matter. It's complicated.

Why does it bother me that Chipmunk just cleans everything up at the end? Why not just rely on that?

A Specific Example
==================

Think through this with me.

Currently, physics objects are created like this.

GameWorld::onBeginPlay() is called. This sets up the scene. It calls createPhysics() on all children (not descendant) PhysicsBodies. Then it recursively calls onBeginPlay() on descendants.

PhysicsBody::createPhysics() is therefore called directly by GameWorld::createPhysicsScene() in the case of root-level bodies. Grandchild and other descendant bodies are created by the ancestors in ways specific to the whims of the ancestors. This is all fine and sensible.

But what if an object is created later, during an arbitrary update? Who calls createPhysics then? The answer is that no-one does! That needs to be fixed, but doing so isn't hard.

Now consider cleanup. Currently, destroyPhysics() is called for bodies but it does nothing. Instead, we rely on the space to clean itself up.

But what if an object needs to delete itself early? In that case, it should call destroyPhysics() in its destructor. Ideally we would do this through a clever std::unique_ptr-style class.

And yet PhysicsBodies sometimes die without wanting to destroy their Chipmunk elements. Therefore we need the ability to turn physics objects over to a "managed" state. So the std::unique_ptr class should have a "detach" feature--not necessarily deleting the object.

The Solutions
=============

One thing I need is better visualization of how Chipmunk is using memory. Right now I have the sloppifying temptation of cpSpaceDestroy(), which cures all ills. If I were to remove this solution, the first question would be: What's the cost? So facilities for inspecting what chipmunk is storing are important.

After that, a physics object pointer solution similar either to std::unique_ptr or std::shared_ptr is necessary to help owners manage objects.