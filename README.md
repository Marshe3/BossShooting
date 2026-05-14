# BossShooting

Unreal Engine 5.7 기반의 4인 listen-server 협동 탑다운 슈터 학습 프로젝트입니다. 목표는 완성형 상용 게임보다, 실제 게임플레이 위에서 Unreal C++ 네트워크 모델을 단계적으로 익히는 것입니다.

## 프로젝트 개요

- 장르: 협동 탑다운 액션/슈터
- 엔진: Unreal Engine 5.7
- 플랫폼: Windows PC
- 네트워크 모델: 최대 4인 listen server
- 구현 방향: 핵심 게임플레이는 C++, 시각 설정과 튜닝은 Blueprint
- 기본 템플릿: UE5 Top Down / Twin Stick 계열 템플릿 기반

## 현재 구현 상태

현재 C++ 기준으로 다음 기반 기능이 들어가 있습니다.

- 플레이어 캐릭터
  - 탑다운 카메라와 WASD 이동
  - 마우스 조준 회전
  - 체력 replication 및 `OnRep` 기반 동기화
  - 서버 권위 `TakeDamage`
  - 기본 무기 자동 장착 및 무기 교체

- 무기 시스템
  - `ABaseWeapon` 기반 공통 무기 actor
  - 서버 RPC 기반 발사 요청
  - 히트스캔 라이플
  - 다중 pellet 히트스캔 샷건 기반
  - projectile 발사 모드
  - 탄약 replication
  - 서버 timer 기반 재장전
  - 무기 pickup 및 1슬롯 교체

- Projectile
  - `ABaseProjectile`
  - 서버 spawn
  - replicated movement
  - 충돌 시 서버 권위 radial damage
  - multicast debug explosion 표시

- 적 / AI
  - `AAlienBase` 공통 적 클래스
  - 체력, 데미지, 사망 처리
  - `AAlienAIController`
  - C++ `UBTTask_FindClosestPlayer`
  - Behavior Tree / Blackboard 기반 추적
  - Rusher 근접 공격
  - Spitter 원거리 산성 projectile 공격

- 웨이브 / 보스 기반
  - `AEnemyWaveTrigger`
  - 서버 overlap 기반 1회 enemy wave spawn
  - `ABossQueen`
  - HP 비율 기반 Phase1 / Phase2 / Phase3 / Dead 상태
  - `CurrentPhase` RepNotify 기반 replication
  - 보스 무적 상태 replication

## 네트워크 원칙

이 프로젝트의 게임플레이는 서버 권위를 기준으로 작성합니다.

- 클라이언트는 입력과 요청만 보냅니다.
- 서버가 발사 가능 여부, 탄약, 재장전, 데미지, pickup claim, 적 spawn, 보스 phase를 결정합니다.
- 지속 상태는 replicated property로 동기화합니다.
- 시각적 1회성 효과는 필요할 때 multicast RPC로 전달합니다.
- AI와 적 공격 판단은 서버에서 실행됩니다.
- remote client에서도 host와 같은 결과가 보이도록 검증합니다.

## 주요 코드 구조

```text
Source/BossShooting/
  Public/
    Character/
      BaseCharacter.h
    Weapon/
      BaseWeapon.h
      BaseProjectile.h
      WeaponPickup.h
    Enemy/
      AlienBase.h
      AlienAIController.h
      AlienSpitter.h
      BossQueen.h
      BTTask_FindClosestPlayer.h
      EnemyWaveTrigger.h
  Private/
    Character/
      BaseCharacter.cpp
    Weapon/
      BaseWeapon.cpp
      BaseProjectile.cpp
      WeaponPickup.cpp
    Enemy/
      AlienBase.cpp
      AlienAIController.cpp
      AlienSpitter.cpp
      BossQueen.cpp
      BTTask_FindClosestPlayer.cpp
      EnemyWaveTrigger.cpp
```

템플릿에서 생성된 `Variant_TwinStick`, `Variant_Strategy` 코드도 남아 있지만, 현재 학습용 핵심 gameplay 코드는 위의 `Character`, `Weapon`, `Enemy` 폴더를 중심으로 진행 중입니다.

## Blueprint 설정 필요 항목

C++ 기반은 준비되어 있지만, 일부 기능은 Unreal Editor에서 Blueprint asset을 만들고 값을 연결해야 실제 PIE에서 확인할 수 있습니다.

- `BP_BaseCharacter`
  - 부모: `BaseCharacter`
  - `DefaultWeaponClass`
  - `MoveAction`, `FireAction`, `ReloadAction`
  - `DefaultMappingContext`

- 무기 Blueprint
  - `BP_Rifle`: 히트스캔 기본 무기
  - `BP_Shotgun`: `PelletCount`, `SpreadAngleDegrees` 설정
  - `BP_GrenadeLauncher`: `FireMode = Projectile`, `ProjectileClass` 설정
  - `BP_GrenadeProjectile`: 부모 `BaseProjectile`

- 적 Blueprint
  - `BP_AlienRusher`: 부모 `AlienBase`
  - `BP_AlienSpitter`: 부모 `AlienSpitter`, `AcidProjectileClass` 설정
  - `BP_AcidProjectile`: 부모 `BaseProjectile`
  - `BP_BossQueen`: 부모 `BossQueen`

- AI asset
  - `BB_Alien`
  - `BT_AlienRusher`
  - `BP_AlienAIController`
  - `NavMeshBoundsVolume`

- 웨이브
  - `BP_EnemyWaveTrigger`
  - `EnemyClasses` 배열에 spawn할 적 BP 지정
  - 필요 시 `SpawnPoints` 지정

## 입력

현재 기준 입력은 다음 흐름을 사용합니다.

- WASD: 이동
- Mouse: 조준 방향
- Left Mouse Button: 발사
- R: 재장전

Enhanced Input asset은 Blueprint / Editor에서 연결해야 합니다.

## 빌드

Unreal Engine 5.7 설치 후 다음 구성을 사용합니다.

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" BossShootingEditor Win64 Development -Project="C:\UEProject\BossShooting\BossShooting.uproject" -WaitMutex
```

최근 작업 기준 C++ 빌드는 성공한 상태로 기록되어 있습니다. 다만 Blueprint asset 생성과 PIE 검증은 기능별로 별도 확인이 필요합니다.

## 테스트 기준

멀티플레이 기능은 다음 방식으로 확인합니다.

- Play As Listen Server
- 가능하면 총 4 players
- host와 remote client 양쪽에서 결과 확인
- 발사, 데미지, 사망, pickup, 재장전, projectile, enemy wave, boss phase 확인

특히 다음은 remote client 화면에서도 확인해야 합니다.

- 캐릭터 위치와 회전 동기화
- 무기 attach / 교체 결과
- 탄약과 재장전 상태
- projectile 이동과 폭발
- 적 이동과 공격 결과
- 보스 phase 전환

## 문서

진행 상태와 설계 기준은 `Docs` 폴더에 정리되어 있습니다.

- `Docs/Handoff_Package_Slim_v2.md`: 현재 구현 상태와 다음 작업 안내
- `Docs/Boss_Coop_Shooter_GDD_v0_3.md`: 게임 기획, 범위, 네트워크 모델
- `Docs/Step5c_Rusher_Melee_Attack_Notes.md`
- `Docs/Step6a_Shotgun_Hitscan_Notes.md`
- `Docs/Step6b_Grenade_Projectile_Notes.md`
- `Docs/Step6c_Weapon_Pickup_Notes.md`
- `Docs/Step6d_Weapon_Reload_Notes.md`
- `Docs/Step7a_Spitter_Ranged_Attack_Notes.md`
- `Docs/Step7b_Enemy_Wave_Trigger_Notes.md`
- `Docs/Step8a_BossQueen_Phase_State_Notes.md`

## 다음 작업

현재 자연스러운 다음 단계는 다음 순서입니다.

1. `BP_BossQueen` 생성 및 phase replication PIE 검증
2. BossQueen Phase1 공격 패턴
3. BossQueen Phase2 소환 / 무적 loop
4. BossQueen Phase3 강화 패턴
5. GameMode / GameState 기반 match flow
6. HUD / UMG
7. 2~4 player listen-server 검증

## 개발 메모

이 프로젝트는 학습 과정 자체가 중요한 프로젝트입니다. 기능을 추가할 때는 어떤 코드가 server, owning client, all clients, simulated proxy 중 어디에서 실행되는지 명확히 기록하고, Blueprint 설정이 필요한 부분은 C++ 작업과 분리해서 남깁니다.
