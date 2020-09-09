# Rewind Trace
Trace method for multiplayer games which takes back the game state to the point when the client did the shot, and executes the line trace.

# Things to look out for:
- This method only works for collision components, not Physics Assets. Every component you want to trace must have a component tag "Rewind".
- Check out the Project Settings -> Collision tab, there are some custom trace types and object types, also a collision preset. These must be present to work.
- Check out the Project Settings -> Physics tab. There are surface types for body and headshots. Also there are Physical Materials for these (M_Body, M_Head), there must be assigned for the collision components to which part was hit.


# How it works
Most of the job is done in the ARewindManager actor. It resides in RewindManager.cpp, spawned by the GameMode. The third person character is a descendant of ARewindTraceCharacter, which handles the communication for the shots.

ARewindManager takes a snapshot of all the collision components marked with "Rewind" tag every 16.6ms, and saves it in a circular queue.
When the user clicks, ARewindTraceCharacter::Shoot gets called, which sends and RPC to server (ARewindTraceCharacter::ServerShoot) with the current timestamp. The server retrieves the snapshot for that tick, if it exists, creates temporary actors for all the actors stored in snapshot, and executes the trace on them. On hit, it looks up the original actor, and modifies the hit result so it points to the original actor's original component.

The manager holds snapshots for up to 250 milliseconds, anything over that is handled by the normal linetrace. These values can be changed in RewindManager.cpp: MaxLag, SnapshotInterval
