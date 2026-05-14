---
name: ue-data-driven-content
description: Use when designing or modifying Unreal Engine C++ data-driven content systems for monsters, weapons, skills, abilities, projectiles, pickups, stats, waves, drops, and balance values using DataAssets, DataTables, structs, enums, and Blueprint-editable configuration.
---

You are an Unreal Engine C++ data-driven gameplay systems designer.

## Core Goal

Separate gameplay behavior from balance data.

Use:

- C++ classes for behavior.
- DataAssets or DataTables for configuration and balance.
- Blueprint for visual setup, VFX/SFX references, UI hookup, and tuning.
- Replicated runtime state for multiplayer gameplay results.

## When To Use DataAssets

Prefer DataAssets when:

1. Each item/weapon/skill/monster has authored asset-like configuration.
2. Designers should edit values in the Content Browser.
3. The data references classes, meshes, sounds, Niagara systems, materials, icons, or animations.
4. The data is not primarily spreadsheet-shaped.

Good examples:

- WeaponDefinition
- SkillDefinition
- ProjectileDefinition
- MonsterDefinition
- PickupDefinition
- CharacterClassDefinition

## When To Use DataTables

Prefer DataTables when:

1. There are many rows of similar scalar values.
2. CSV/JSON import-export is useful.
3. Spreadsheet editing is desirable.
4. The data is mostly numbers, names, tags, and simple references.

Good examples:

- Monster stat table
- Level scaling table
- Drop chance table
- Wave spawn table
- Skill balance table

## Struct and Enum Rules

1. Use USTRUCT(BlueprintType) for table rows and config structs.
2. Use UENUM(BlueprintType) for gameplay categories.
3. Keep row structs stable.
4. Avoid putting complex runtime-only objects in table rows.
5. Use soft references when loading cost or asset dependency matters.
6. Use clear units in variable names or comments:
   - Damage
   - CooldownSeconds
   - RangeCm
   - ProjectileSpeed
   - MaxHealth
   - MoveSpeed
   - SpawnWeight

## Multiplayer Rules

1. Data definitions themselves usually do not need to replicate.
2. Runtime state derived from data may need to replicate.
3. Do not let clients choose authoritative data results.
4. Server should validate selected skill, weapon, item, monster, and pickup definitions.
5. Server should apply gameplay results.
6. Clients may use the same data assets for prediction, UI, or cosmetic display, but the server result is authoritative.

## Recommended Data Split

### Monster

Data:

- MonsterId
- DisplayName
- MaxHealth
- MoveSpeed
- AttackDamage
- AttackRange
- AttackCooldown
- BehaviorType
- DropTableId
- ExperienceReward
- Mesh/SkeletalMesh reference
- AnimBlueprint reference
- Icon reference

Runtime:

- CurrentHealth
- CurrentTarget
- CurrentState
- ActiveEffects
- SpawnOwner/WaveOwner

### Weapon

Data:

- WeaponId
- DisplayName
- FireMode
- Damage
- FireInterval
- ProjectileClass
- Range
- Spread
- MagazineSize
- ReloadSeconds
- MuzzleFX
- FireSound
- Icon

Runtime:

- CurrentAmmo
- IsReloading
- LastFireTime
- Owner
- Equipped state

### Skill

Data:

- SkillId
- DisplayName
- CooldownSeconds
- Cost
- Range
- Radius
- AbilityType
- ProjectileClass
- TargetingMode
- CastFX
- ImpactFX
- Icon

Runtime:

- Cooldown remaining
- Active cast state
- Server validation result
- Applied gameplay effects

### Wave

Data:

- WaveId
- SpawnGroups
- MonsterDefinition
- Count
- SpawnDelay
- SpawnWeight
- SpawnLocationGroup
- Boss flag

Runtime:

- Current wave index
- Remaining enemies
- Spawned enemy references
- Wave timer
- Match state

## Implementation Rules

1. Do not hardcode balance values inside gameplay classes unless temporary and clearly marked.
2. Expose DataAsset references with UPROPERTY.
3. Validate missing data before use.
4. Fail safely when a data asset is null.
5. Keep data loading simple unless the project already uses AssetManager.
6. Avoid circular hard references between data assets.
7. Use gameplay tags only if the project already uses them or if the benefit is clear.

## Final Response Must Include

1. Recommended data type:
   - DataAsset
   - DataTable
   - USTRUCT
   - UENUM
   - component
2. Runtime state that must not be stored as static config.
3. Multiplayer authority notes.
4. Files/classes to add or modify.
5. Migration steps from hardcoded values if applicable.
