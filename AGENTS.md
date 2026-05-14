# Project Agent Guide

This is an Unreal Engine C++ top-down action/shooter project.

## Project Model

- Unreal Engine C++ project.
- Top-down action/shooter gameplay.
- 4-player listen-server multiplayer.
- Core gameplay should be implemented in C++.
- Blueprint should be used for visual setup, animation hookup, VFX/SFX hookup, UI hookup, and tuning values.

## Multiplayer Model

This project is a 4-player listen-server multiplayer game.

Rules:
- The server is authoritative for gameplay.
- The listen-server host is both server and local player.
- Code must work for remote clients, not only for the host.
- Combat, damage, death, pickups, enemy AI, score, inventory, projectile hits, ability activation, cooldowns, and match state must be server-authoritative.
- Clients may request actions through Server RPCs.
- Persistent gameplay state should use replicated properties.
- Client-side reactions to replicated state should use RepNotify where appropriate.
- Cosmetic one-shot events may use Client RPCs or NetMulticast RPCs depending on who should see them.
- Do not trust client-provided damage, hit results, cooldowns, rewards, item claims, or spawn decisions.
- Avoid using GetPlayerController(0) for multiplayer gameplay logic.
- Always explain which machine executes important code: server, owning client, all clients, or simulated proxies.

## Architecture Rules

- Prefer ActorComponents for reusable gameplay logic.
- Good candidates for components:
  - HealthComponent
  - CombatComponent
  - WeaponComponent
  - AbilityComponent
  - InventoryComponent
  - InteractionComponent
- Do not put unrelated gameplay logic into the Character class.
- Avoid Tick unless continuous per-frame behavior is truly required.
- Expose gameplay tuning values with UPROPERTY.
- Do not hardcode balance values when a DataAsset or DataTable would be better.
- Keep headers small.
- Prefer forward declarations in headers where possible.
- Put implementation details in .cpp files.
- Add Build.cs dependencies only when required.

## Unreal Reflection Rules

- Use UCLASS, USTRUCT, UENUM, UPROPERTY, and UFUNCTION correctly.
- Do not forget GENERATED_BODY().
- Replicated properties must be registered in GetLifetimeReplicatedProps.
- UObject references that need garbage collection tracking should usually be UPROPERTY.
- Do not use raw owning pointers for UObjects.

## Networking Rules

- Authoritative gameplay state must be modified only on the server.
- Projectiles that deal damage must be spawned by the server.
- Damage must be applied by the server.
- Enemy AI should run on the server.
- Pickups should be claimed by the server.
- UI should read replicated state or be updated by owner-only client events.
- Reliable RPCs should be reserved for important low-frequency events.
- Avoid Reliable RPC spam for repeated firing, movement-like behavior, or cosmetic-only effects.

## Testing

- Test using Play As Listen Server.
- Test with 4 total players when possible.
- Verify behavior as host and as remote client.
- Multiplayer work is not complete until remote-client behavior has been checked.
- Check combat, death, pickups, projectile hits, score, ability cooldowns, and UI synchronization.
- Clearly state anything that could not be verified.

## Done Means

- Code compiles, or the reason it cannot be verified is clearly stated.
- UnrealHeaderTool-sensitive mistakes are checked.
- Build.cs dependencies are checked.
- Replication and RPC ownership are checked.
- Changed files are summarized.
- Remaining multiplayer risks are listed.

## Project Documents

Before starting non-trivial gameplay, AI, multiplayer, UI, or architecture work, read:

1. `Docs/Handoff_Package_Slim_v2.md`
2. `Docs/Boss_Coop_Shooter_GDD_v0_3.md`

Use the handoff document as the current implementation state and next-step guide.
Use the GDD as the design, scope, learning-goal, and network-model source of truth.

Important:
- The handoff document may be more up to date than the GDD for current progress.
- The GDD defines project identity and scope.
- Do not expand scope beyond the GDD unless explicitly asked.
- If Blueprint `.uasset` state is relevant, rely on the handoff document because binary assets cannot be reliably inspected as text.