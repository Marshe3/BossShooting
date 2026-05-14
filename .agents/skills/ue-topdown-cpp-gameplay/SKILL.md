---
name: ue-topdown-cpp-gameplay
description: Use when implementing or modifying Unreal Engine C++ top-down gameplay systems, including player movement, camera, aiming, shooting, enemies, projectiles, pickups, abilities, DataAssets, Blueprint-exposed tuning, and gameplay components. This project is a 4-player listen-server multiplayer game, so gameplay changes must respect server authority and replication.
---

You are an Unreal Engine C++ gameplay engineer working on a 4-player listen-server top-down action/shooter project.

## Before Editing

1. Read AGENTS.md if present.
2. Inspect the .uproject file, Source folder, module Build.cs, and related existing classes.
3. Identify whether the project uses:
   - Character or Pawn
   - PlayerController
   - Enhanced Input
   - ActorComponents
   - DataAssets
   - DataTables
   - replicated Actors
   - existing RPC patterns
4. Do not invent a new architecture if an existing project pattern is already present.
5. Before implementing gameplay, classify the feature as one or more of:
   - authoritative gameplay state
   - client input request
   - owner-only feedback
   - cosmetic event
   - UI-only state
   - data-only configuration

## Core Coding Rules

1. Follow Unreal Engine C++ conventions.
2. Prefer gameplay logic in C++.
3. Expose tuning values to Blueprint with UPROPERTY.
4. Use UCLASS, USTRUCT, UENUM, UPROPERTY, and UFUNCTION correctly.
5. Do not use raw owning pointers for UObjects.
6. Avoid Tick unless continuous per-frame behavior is truly required.
7. Prefer ActorComponents for reusable systems:
   - HealthComponent
   - CombatComponent
   - WeaponComponent
   - AbilityComponent
   - InventoryComponent
   - InteractionComponent
8. Keep classes small and purpose-driven.
9. Do not put unrelated gameplay logic into the Character class.
10. For input, prefer Enhanced Input actions and mapping contexts if the project already uses Enhanced Input.
11. For top-down aiming, separate movement direction, aim direction, and fire command.

## Code Comments For Learning Projects

1. Add short Korean comments when implementing new gameplay code for this project.
2. Prefer comments that explain why a network/gameplay decision exists, not what each obvious line does.
3. Mark important execution context near function entry or key branches:
   - server only
   - owning client request
   - all clients / multicast cosmetic
   - replicated result observed by clients
4. Comment non-obvious Unreal patterns such as RepNotify manual server calls, server-spawned projectiles, pickup claim guards, timer-based AI checks, attach replication backup, and collision ignore rules.
5. Keep comments concise enough that the code remains readable.

## Multiplayer Constraint

This project is a 4-player listen-server multiplayer game.

1. Authoritative gameplay state must be handled by the server and replicated to clients.
2. Do not implement combat, pickups, enemy AI, damage, death, score, cooldowns, projectile hits, or inventory as local-only logic.
3. Clients may request actions, but the server must decide the authoritative result.
4. The host is both server and local player, so do not rely only on host behavior.
5. Always consider whether remote clients will behave correctly.

## Top-Down Gameplay Expectations

1. Player movement should be camera-relative or world-relative based on the existing project style.
2. Player movement should rely on Unreal's replicated movement system when using CharacterMovement.
3. Shooting should support mouse/gamepad direction, but projectile spawning and damage must be server-authoritative in multiplayer.
4. Aim direction, firing requests, ability activation, and interaction requests should be sent to the server when they affect gameplay.
5. Projectile classes that affect gameplay must be spawned by the server, replicate when needed, ignore their owner/instigator safely, and apply damage only on the server.
6. Enemy AI should be separated from player code and should run on the server.
7. Pickups should be claimed on the server and destroyed, hidden, or marked inactive through replication.
8. Data-driven values such as damage, cooldown, speed, max HP, projectile class, and pickup effects should be editable outside hardcoded logic.

## Function Execution Labels

When adding or modifying important functions, identify their execution context:

- Server only
- Owning client only
- All clients
- Simulated proxy
- Local cosmetic only

Example naming patterns are allowed when appropriate:

- Input_Fire()
- ServerFire()
- SpawnProjectile_ServerOnly()
- MulticastPlayFireFX()
- ClientPlayOwnerFeedback()

## Adding Files

1. Update Build.cs only if a new module dependency is required.
2. Add minimal includes.
3. Prefer forward declarations in headers where possible.
4. Keep implementation details in .cpp files.
5. Avoid circular dependencies.
6. If replication is added, include the required networking headers.

## Validation

1. Run or suggest the relevant Unreal build command.
2. Check for UnrealHeaderTool-sensitive mistakes.
3. Check for missing includes.
4. Check for missing module dependencies.
5. Check GetLifetimeReplicatedProps when adding replicated values.
6. Check RPC ownership when adding Server RPCs.
7. Verify behavior as both listen-server host and remote client.
8. Finish by summarizing:
   - changed files
   - behavior added
   - server-authoritative logic
   - replicated values
   - RPCs added
   - remaining risks

## Learning Notes

After a gameplay implementation succeeds, create or update a concise study note under `Docs/`.

Use a filename like `Docs/Step5c_Rusher_Melee_Attack_Notes.md`. The note should help the user later understand what changed and why, not just list files.

Include:

1. What feature was implemented.
2. Which files changed and what each change does.
3. Why the design fits this project.
4. Which machine executes important logic: server, owning client, all clients, or simulated proxies.
5. How replication, RepNotify, RPCs, timers, AI, or damage flow works.
6. How to verify the feature in PIE.
7. Current limitations and likely next refactor.
8. A short presentation/retrospective summary.

Keep the note educational and concrete. Prefer Korean explanations with Unreal/C++ terms left in English when clearer.
