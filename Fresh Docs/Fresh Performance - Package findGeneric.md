Fresh Performance: Package::findGeneric()
-----------------------------------------

This function continues to be expensive. I need to optimize it sharply.

There are several sources of expense.

The overall algorithm is O(n) on every member of list. For each member:

1.	A (cheap, I hope) null pointer check is performed.
2.	A slow O(n) string name comparison is performed.
3.	If that passes (which should be rare), a slow O(n) dynamic_cast<>-style class comparison is performed.

A major obstacle to refining this function is that it's so essential that any modifications have a catastrophic behavioral effect. Also, these substeps are all inlined and therefore don't show up individually in the profile. It's a mystery which is most expensive.

There are several possible strategies to optimization.

1.	Ensure that null pointers never exist, so the null check is never required.
2.	Improve the speed of name comparisons, for instance by using StringTabulated.
3.	Optimize ClassInfo::isKindOf(). See e.g. http://www.stroustrup.com/fast_dynamic_casting.pdf.
4.	Improve upon the O(n) performance of the overall sweep.
5.	Reduce n for the overall sweep.
6.	Simply reduce the overall use of findGeneric(), especially in problematic circumstances (e.g. creating CollisionEffects).

#1 is trivial to implement, since I actually had it this way before. Things did seem to be faster then. So I'll try it again, pessimistically.

#2 is fairly hard and not a clear win. Object names change fairly often (take name $^_parsing for example), so the map is liable to get littered. There's a fair engineering cost in terms of modifying ObjectId and such.

#3 is pretty easy given Stroustroup's prime number method. Might do this just because isKindOf() tends to pop up a lot.

#4 -- I don't see how to do this. If the requesting information had a clear ordinal form I could sort members by that ordinal and do a bisect. But it's a string object name and a ClassInfo belonging to, and to some extent inclusive of, a larger class scheme. I don't see how to ordinalize this.

#5 actually seems promising. One option would be to break up certain packages by common base classes, in effect keeping a few separate member lists each keyed to a different base class. Then when you get a request for a given class, you can immediately eliminate any list keyed on a base class from which the requested class does not derive. For some of the worst offending packages---like assets---this could be just the ticket.

#6 is of course good advice, and might be important for CollisionEffects in particular, but this function is showing up everywhere, so I can't throw the whole burden to the user.

------------

Incredible. Trying #1---basically reverting to the policy of tidy()ing at least before findGeneric()---fixed the performance issue entirely. I don't really understand why---I didn't even remove the SwitchPtr test. I don't get it. But I'm glad.