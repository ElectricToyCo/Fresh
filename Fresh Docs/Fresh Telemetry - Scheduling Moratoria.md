Fresh Telemetry: Scheduling Moratoria
-------------------------------------

# Executive Summary

Change startSession.php to read "moratorium 15" to instruct all clients to ignore telemetry for 15 minutes after the moment when they contact the server, or "moratorium @YYYY-MM-DD HH:MM:SS" for an absolute end-of-moratorium time.

# Rationale

The telemetry system comes with substantial costs.

*	Server bandwidth, CPU, and storage space per user, per session, and even per event.
*	Large (on the order of a second) delays on the client side, particularly during context starts (usually the beginnings of levels).

When relatively few users are involved, or when the cost of client-side delays are outweighed by the benefits of telemetry, such as during beta testing, then these costs are not prohibitive. But when a game is in full release, when telemetry is no longer needed or the cost of gathering data is too high, then it is sometimes helpful to be able to turn telemetry off. This may be done by building Fresh (and the game) with FRESH_TELEMETRY_ENABLED undefined (or defined to '0'). But sometimes a "live" version of the game may have telemetry permanently enabled. How, then, do you disable telemetry for an already-live build?

# Moratoria

A Telemetry Moratorium is a period during which telemetry clients are instructed *not to use or telemetry or even contact the server* for a period of time. When a moratorium is in effect, clients will still contact the server during their initial run (or whenever app preferences are erased), but will receive a cheap, short reply that tells them to stop working with telemetry data and not to contact the server again for a specified period.

# Scheduling a Moratorium

Clients always contact http://<game-server>/<game-telemetry-folder>/startSession.php when initiating their telemetry session. Scheduling a moratorium simply involves replacing the startSession.php file on the server with a brief file announcing the moratorium. An example of this file may be found as moratorium_template.php.

A startSession.php file that has been modified to declare a moratorium has this form:

	moratorium <time-reference>

The time reference may be either relative or absolute. It indicates a time before which the client should not contact the server. Clients only contact the server (in this sense) at the beginning of a "session", which is normally when an app is executed. Therefore there is by no means a guarantee that the client will contact the server when the moratorium is over: only that it definitely won't contact the server *before* the moratorium is over.

## Relative Moratorium

A relative moratorium simply declares the number of minutes from the client's current time that it should wait before contacting the server again. E.g:

	moratorium 10

Instructs the client to ignore telemetry for the current session and not to contact the server until at least 10 minutes have passed.

## Absolute Moratorium

An absolute moratorium declares a UTC time at which the current moratorium will end. Example:

	moratorium @2015-01-01 13:30:00

This output schedules a moratorium that will last until Jan 1, 2015 at 1:30 PM, Greenwich Mean Time.

## Caveats

No matter what the "moratorium" time reference declares, the existance of the 'moratorium' keyword at the beginning of startServer.php's return output will cause the client to ignore telemetry for the current session. That is, even if you say "moratorium 0" or "moratorium @1999-12-31 23:59:59" (that is, in the past), the current telemetry session will still be silenced. The client will contact the server again when it next executes, however.