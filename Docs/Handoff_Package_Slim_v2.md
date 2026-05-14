# 핸드오프 패키지 (슬림 v2) — 언리얼 5.7 멀티플레이 학습 프로젝트

> **이 문서 사용법**: C++ 코드 파일들을 프로젝트에 직접 연결하는 경우 사용. AI가 코드를 라이브로 읽으므로 문서엔 메타 정보만 담음.
>
> 새 AI 채팅창에:
> 1. 프로젝트 폴더(C++ 소스 + GDD)를 연결
> 2. 이 파일 (`Handoff_Package_Slim_v2.md`)을 첨부
> 3. 아래 "도입 메시지" 섹션을 첫 메시지로 복사

---

## ▶ 도입 메시지 (이 부분만 복사해서 새 AI에 첫 메시지로 보내기)

안녕. 나는 GCC사관학교 개인 과제로 **언리얼 엔진 5.7 멀티플레이 학습 프로젝트**를 진행 중이야. 이전 AI 채팅에서 토큰 한도가 다 차서 새 채팅으로 옮겨왔고, 그 사이에 다른 AI 어시스턴트(Codex)랑 추가 작업도 좀 했어. 그 내용도 이 문서에 반영돼 있어.

이 채팅에는 다음이 연결돼 있어:
1. **프로젝트 폴더** — C++ 소스 코드 직접 읽을 수 있음 (`Source/BossShooting/Public|Private/...`)
2. **`Boss_Coop_Shooter_GDD_v0_3.md`** — 게임 기획서 v0.3 (세계관·캐릭터·화면기획 포함)
3. **`Handoff_Package_Slim_v2.md`** (이 파일) — 진행 상황 + 최근 검증 결과 + 작업 방식

작업 시작 전에:
1. 프로젝트의 C++ 파일들을 한 번 훑어봐 (`Character/BaseCharacter`, `Weapon/BaseWeapon`, `Enemy/AlienBase`, `Enemy/AlienAIController`, `Enemy/BTTask_FindClosestPlayer`)
2. 이 문서의 **§4 진행 상태**와 **§6 최근 검증 결과**, **§7 다음 작업 순서**를 읽어
3. "Step 5c의 어디부터 이어갈까?"를 나한테 물어줘

너의 역할은 이 학습 프로젝트의 **멘토**야. 작업 방식은 §5에 정리해뒀어.

---

## 1. 프로젝트 한 줄 요약

탑뷰 시점, 1~4인 협동, 외계인 여왕과 맞서는 **2주짜리 학습 프로젝트**. *Alien Swarm* 분위기. **외형은 슈터지만 본질은 언리얼 네트워크 시스템 학습 키트**. 게임 완성도보다 "이거 만들면서 뭘 배웠나"가 1차 목표.

---

## 2. 개발 환경

| 항목 | 값 |
|------|----|
| 엔진 | Unreal Engine **5.7** |
| 플랫폼 | Windows PC (Listen Server, 최대 4인) |
| 개발 기간 | 2026년 5월 21일까지 |
| 베이스 템플릿 | UE5 Top Down (Variant_TwinStick 활용) |
| 저장소 | GitHub + Git LFS (`https://github.com/Marshe3/BossShooting`) |
| 모듈 의존성 | `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, **`AIModule`, `GameplayTasks`** (BTTask용) |
| 프로젝트 모듈명 | `BossShooting` (API 매크로: `BOSSSHOOTING_API`) |

> ⚠️ Build.cs의 `PublicDependencyModuleNames`에 `AIModule`, `GameplayTasks`가 들어가 있는지 확인 필요 (BTTask C++ 추가하면서 코덱스가 추가했을 것)

### C++ 소스 구조 (프로젝트에서 직접 읽을 것)

```
Source/BossShooting/
├── Public/
│   ├── Character/BaseCharacter.h
│   ├── Weapon/BaseWeapon.h
│   └── Enemy/
│       ├── AlienBase.h
│       ├── AlienAIController.h
│       └── BTTask_FindClosestPlayer.h   ★ Codex가 추가 (BP Task에서 C++로 전환)
└── Private/
    ├── Character/BaseCharacter.cpp
    ├── Weapon/BaseWeapon.cpp
    └── Enemy/
        ├── AlienBase.cpp
        ├── AlienAIController.cpp
        └── BTTask_FindClosestPlayer.cpp  ★ Codex가 추가
```

### BP 자산 (.uasset 바이너리, AI 못 읽음 — 텍스트로 셋업 상태만 남김)

```
Content/Game/
├── Blueprints/
│   ├── Character/BP_BaseCharacter
│   │   └─ Input 카테고리: IMC_Default, IA_Move, IA_Fire 자산 할당됨
│   │   └─ Weapon 카테고리: Default Weapon Class = BP_Rifle
│   ├── Framework/BP_BossGameMode
│   │   └─ Default Pawn Class = BP_BaseCharacter
│   ├── Weapon/BP_Rifle
│   │   └─ 부모: BaseWeapon
│   │   └─ WeaponMesh, Damage 20, FireRate 0.15, MaxAmmo 30, AttachSocketName "hand_r"
│   └── Enemy/
│       ├── BP_AlienRusher
│       │   └─ 부모: AlienBase
│       │   └─ Mesh: SKM_Manny (Z=-90, Yaw=-90)
│       │   └─ MaxHealth 30
│       │   └─ AI Controller Class = BP_AlienAIController ✅
│       └── BP_AlienAIController
│           └─ 부모: AlienAIController
│           └─ Behavior Tree Asset = BT_AlienRusher ✅
├── Input/
│   ├── IA_Move (Value Type: Axis2D)
│   ├── IA_Fire (Value Type: Digital/Bool, 좌클릭)
│   └── IMC_Default
│       └─ IA_Move 매핑: W(Swizzle YXZ), S(Swizzle YXZ + Negate), D, A(Negate)
│       └─ IA_Fire 매핑: Left Mouse Button
├── AI/
│   ├── BB_Alien (Blackboard) ✅
│   │   └─ 키: TargetActor (Type: Object → BaseCharacter 또는 Actor)
│   └── BT_AlienRusher ✅
│       └─ Blackboard = BB_Alien
│       └─ Root → Sequence → Find Closest Player(C++ Task) → Move To(TargetActor)
└── Maps/Lv1Test01
    └─ NavMeshBoundsVolume + RecastNavMesh 있음 ✅
```

**삭제된 것** (이전 핸드오프 v1에선 있었으나 Codex 작업으로 제거됨):
- ~~`Content/Game/AI/BTTask_FindClosestPlayer` (BP Task)~~ — C++ 버전으로 대체

---

## 3. 사용자 배경

- **풀스택 백엔드 경험자** (Java/Spring Boot, React)
- **C++ 메모리 관리 학습 중** (`shared_ptr`, `weak_ptr`, 순환 참조)
- 언리얼은 학원 과정에서 학습 중 (블루프린트 / 언리얼 C++ 둘 다)
- 작업 PC 여러 대 — Git으로 동기화
- **BP 노드 그래프 작업에 익숙하지 않음** — 그래서 BTTask도 BP에서 막혀서 C++로 전환함
- **C++ 작업은 비교적 익숙** — 함수 구현·코드 수정 안내가 BP 노드 안내보다 효율적

---

## 4. 현재 진행 상태

### ✅ 완료된 Step (1~5a)

| Step | 내용 | 핵심 학습 |
|------|------|-----------|
| **1** | UE 5.7 셋업, Git LFS, `ABaseCharacter` 골격, 탑뷰 카메라(SpringArm 1200/-60도), 2인 PIE 스모크 테스트 | 멀티플레이 기본 플래그 |
| **2** | Health 시스템 — `Replicated` + `RepNotify` + `TakeDamage` + `DebugSelfDamage` 콘솔 명령 | `DOREPLIFETIME`, RepNotify 자동/수동 호출 차이, `HasAuthority` |
| **3a** | Enhanced Input + WASD 이동 | `AddMovementInput`으로 CMC 자동 동기화, Enhanced Input 3단 구조 |
| **3b** | 마우스 조준 회전 + `Server_SetAimRotation` Unreliable RPC + `DeprojectMousePositionToWorld` + 평면 교차 | Server RPC 첫 등장, `IsLocallyControlled` vs `HasAuthority`, 콜리전 의존 X |
| **4a** | `ABaseWeapon` 별도 액터, attach + `Replicated` 포인터, `OnRep_OwningCharacter`에서 클라 측 attach 보강, `SetOwner` | RepNotify의 두 번째 역할, Owner = RPC 통로 |
| **4b** | 발사 — `Server_Fire`(Reliable) + `Multicast_PlayFireFX`(Unreliable) + 라인트레이스 히트스캔 | RPC 세 종류 한 자리 + Reliable/Unreliable 결정 기준 |
| **5a** | `AAlienBase`, Health 패턴 재사용, `HandleDeath` + `Multicast_OnDeath` 분리, Ragdoll 사망 | 사망 처리의 두 갈래, 코드 재사용성 |

### ✅ 완료 — Step 5b: AIController + BehaviorTree + Blackboard

**완료/확인된 부분**:
- ✅ `AAlienAIController` C++ 클래스 (`OnPossess`에서 BT 실행)
- ✅ `AAlienBase`에 `AIControllerClass`, `AutoPossessAI` 설정
- ✅ `BB_Alien` Blackboard 자산 (키: TargetActor)
- ✅ **`BTTask_FindClosestPlayer` C++ Task** (Codex가 작성, 헤더+cpp) — 기본 Blackboard Key 값을 `TargetActor`로 박아둠
- ✅ `BT_AlienRusher` 자산 생성 및 노드 트리 구성
- ✅ `BP_AlienAIController` BP 자산 생성 + Behavior Tree Asset = `BT_AlienRusher`
- ✅ `BP_AlienRusher` AI Controller Class = `BP_AlienAIController`
- ✅ `Lv1Test01` 맵에 `NavMeshBoundsVolume` 및 `RecastNavMesh` 존재

**검증 결과**:
- ✅ PIE 멀티플레이에서 외계인이 플레이어를 추격함
- ✅ 적과 멀어지면 더 가까운 다른 플레이어 쪽으로 타겟이 바뀌는 동작 확인
- ✅ 호스트/클라이언트 두 화면에서 같은 적 이동 결과가 보임
- ⚠️ 로그 출력은 확인하지 못했음. 다만 현재 구조상 AIController/BehaviorTree 판단은 서버에서 실행되고, 클라이언트는 `AAlienBase`의 replicated movement 결과를 보는 모델로 판단 가능.

**다음 단계**: Step 6c — BP_WeaponPickup 에셋 생성/튜닝 및 PIE 검증. C++ pickup 기반은 추가됐고, 다음은 BP에서 WeaponClass를 지정해 맵에 배치하는 단계.

---

## 5. 작업 방식 (멘토에게)

- **학습 중심 프로젝트**임을 항상 기억해줘. 완성도보다 "왜 이렇게 짜는가"의 설명이 중요.
- **단계별 진행** — 한 Step씩 차근차근. 한 번에 너무 많은 코드 던지지 말고.
- **코드 + 설명 + 검증 단계** 세트로. "이거 짜고 → 이건 왜 그런지 → 이렇게 확인" 흐름.
- **학습 포인트 명시** — 새 개념 등장하면 항상 "왜 이렇게 하나" 깊이 있게 설명.
- **흔한 실수 미리 알려주기** — 단계 끝에 자주 빠뜨리는 함정.
- **반말 + 캐주얼 톤** — 한국어로. 코딩 용어는 영어 그대로 OK.
- **백엔드 경험 활용** — Java/Spring 비유 가능하면 사용 (예: "ERD에서 엔티티 관계랑 비슷한 결").
- **GDD 정체성 유지** — 화려한 VFX/사운드/GAS 등은 의도적 비목표. 본 스코프 보호.
- **BP 셋업은 단계별 안내** — 사용자가 BP 그래프 익숙치 않으므로 "이 탭 열기 → 이 슬롯 확인 → 이 자산 할당" 식으로 풀어 설명. 노드 그래프는 가능하면 C++로 우회.

---

## 6. 최근 검증 결과 — Step 5b

상황: Step 5b의 에셋 연결과 NavMesh 배치를 완료했고, PIE 멀티플레이에서 러셔 추격 동작을 확인함.

현재 연결 상태:
1. `BP_AlienRusher` → AI Controller Class = `BP_AlienAIController`
2. `BP_AlienAIController` → Behavior Tree Asset = `BT_AlienRusher`
3. `BT_AlienRusher` → Blackboard = `BB_Alien`
4. `BB_Alien.TargetActor` → `Find Closest Player` C++ Task가 갱신
5. `Move To` → `TargetActor`를 향해 이동
6. `Lv1Test01` → `NavMeshBoundsVolume` + `RecastNavMesh` 존재

연결 체인:
> `BP_AlienRusher` → `BP_AlienAIController` → `BT_AlienRusher` → `BB_Alien.TargetActor`

검증 근거:
- 플레이어가 멀어지면 러셔가 더 가까운 다른 플레이어로 타겟을 바꿔 추격함.
- 호스트 화면과 remote client 화면에서 같은 이동 결과가 보임.
- 이는 서버가 AI 판단/이동 명령을 수행하고, 클라이언트는 결과 위치/회전을 replicated movement로 보는 모델과 일치함.

아직 직접 확인하지 못한 것:
- `UE_LOG` 또는 Behavior Tree Debugger로 “BTTask가 서버 월드에서만 실행됨”을 로그로 못 박는 것.
- 발표 자료용으로 확실한 증거가 필요하면 `BTTask_FindClosestPlayer`에 임시 `GEngine->AddOnScreenDebugMessage` 또는 `UE_LOG`를 추가해 `HasAuthority=1`을 확인하면 됨.

---

## 7. 다음 작업 순서

### Step 5c 완료 — 러셔 근접 공격

Step 5b는 동작 확인 완료. Step 5c는 “따라가기만 하는 적”에서 “서버 권위로 공격/데미지를 주는 적”으로 확장하는 단계.

**완료된 코드 작업**:
- `ABaseCharacter::IsDead()` 추가 — AI가 체력 0인 플레이어를 공격 대상으로 고르지 않게 하기 위한 조회 함수.
- `AAlienBase`에 `Alien|Melee` 튜닝값 추가:
  - `bCanMeleeAttack`
  - `MeleeAttackRange` 기본 120
  - `MeleeAttackDamage` 기본 10
  - `MeleeAttackCooldown` 기본 1.0초
  - `MeleeAttackCheckInterval` 기본 0.2초
- 서버에서만 근접 공격 타이머 시작.
- 서버가 범위 안의 가장 가까운 살아있는 `ABaseCharacter`를 찾고, 쿨다운이 끝났으면 `TakeDamage` 호출.
- 외계인 사망 시 근접 공격 타이머 정지.
- `BTTask_FindClosestPlayer`에 있던 null 포인터 위험 로그 정리.
- `BossShootingEditor Win64 Development` 빌드 성공.

**권장 구현 흐름**
1. `AAlienBase` 또는 러셔 전용 자식에 근접 공격 튜닝값 추가 ✅
   - AttackRange: 예) 120
   - AttackDamage: 예) 10
   - AttackCooldown: 예) 1.0초
2. 서버에서만 공격 가능 여부 판정 ✅
   - 타겟이 유효한가
   - 거리 안인가
   - 쿨다운이 끝났는가
   - 타겟이 살아있는가
3. 서버에서 기존 `TakeDamage` 흐름으로 데미지 적용 ✅
4. 공격 연출은 나중에 `NetMulticast` 또는 AnimMontage hookup으로 분리
5. BT는 Step 5c에서 `Selector` 구조로 확장
   - 공격 가능한 경우: 공격 Task
   - 아니면: `Find Closest Player` → `Move To`

**검증 결과**:
- PIE 멀티플레이에서 러셔 근접 공격이 정상 동작함.
- 러셔가 붙으면 플레이어 체력이 감소함.
- 현재는 별도 공격 애니메이션/BT 공격 Task 없이 서버 타이머로 데미지를 넣는 최소 버전. 연출과 BT Selector 구조는 다음 확장 포인트.

> 학습 포인트: Step 5b는 “AI 판단 위치”, Step 5c는 “AI가 만든 gameplay result도 서버 권위여야 한다”를 확인하는 단계.

### Step 5b 완료된 작업 기록

**1. `BT_AlienRusher` 자산 생성**
- 콘텐츠 브라우저 `Content/Game/AI/` 우클릭 → Artificial Intelligence → Behavior Tree
- 이름: `BT_AlienRusher`
- 더블클릭 → Details에서 **Blackboard Asset = `BB_Alien`** 지정

**2. 노드 트리 구성**
```
Root
 └─ Sequence
     ├─ Find Closest Player (C++ Task, 검색해서 추가)
     │    └─ Blackboard Key: TargetActor (Task 디테일에서)
     └─ Move To (내장 Task)
          └─ Blackboard Key: TargetActor
          └─ Acceptable Radius: 100
```

> 단순화: Step 3a 예고에선 `Selector → Sequence → ... → Wait` 였는데, 일단 `Sequence` 하나로 시작. 공격 분기(Step 5c)에서 `Selector`가 의미 있어짐.

**3. `BP_AlienAIController` 열어서 Behavior Tree Asset 슬롯에 `BT_AlienRusher` 할당**

**4. `BP_AlienRusher`의 AI Controller Class 변경**
- Class Defaults → AI Controller Class = **`BP_AlienAIController`** (현재 C++ `AlienAIController`로 잡혀있음, BP 버전으로 바꿔야 BT가 실행됨)
- Auto Possess AI = `Placed in World or Spawned` (이미 코드에서 박았지만 BP에서도 확인)

**5. 🚨 `NavMeshBoundsVolume`을 레벨에 배치**
- Place Actors 패널 → 검색 `NavMesh` → NavMeshBoundsVolume 드래그
- 스케일을 외계인·플레이어 움직일 영역 전체 덮게 (예: `Scale 100, 100, 10`)
- 키보드 `P` 키 → **녹색 영역**이 보이면 정상. **없으면 BT의 Move To가 매번 실패**.

**6. 검증**
- 두 PIE 창에서 외계인이 플레이어 추격
- 두 사람이 다른 위치면 더 가까운 쪽으로 이동
- `Window > Debug > Behavior Tree` 디버거 켜기:
  - 호스트 창에서: BT 실행 중 (녹색 흐름) ← AI가 서버에서 돈다는 직접 증거
  - 클라 창에서: BT 비어있음 ← 클라엔 AI 없음, 결과만 리플리케이션

### Step 5c (그 다음)
- 외계인 근접 공격 (플레이어와 거리 100 이내면 공격, 데미지 적용)
- BT에 공격 분기 추가 — `Selector` 아래 [공격 Sequence, 이동 Sequence] 두 갈래 (여기서 Selector의 진가)
- 외계인 사망 → 점수/카운트 처리 (`GameState` 활용)

### Step 6a 진행 중 — 샷건용 다발 히트스캔 기반

**완료된 코드 작업**:
- `ABaseWeapon`에 `PelletCount`, `SpreadAngleDegrees` 추가.
- 기본값은 `PelletCount=1`, `SpreadAngleDegrees=0`이라 기존 `BP_Rifle` 동작은 유지됨.
- `Server_Fire`는 이제 클라이언트가 보낸 TraceStart/TraceEnd를 그대로 쓰지 않고, 서버의 `OwningCharacter` 위치에서 TraceStart를 계산함.
- 서버가 pellet 방향/spread를 생성하고 각 pellet마다 라인트레이스/데미지를 처리함.
- 디버그 라인은 한 번의 `NetMulticast` RPC에 `TraceEnds`, `HitActors` 배열을 담아 모든 클라이언트에 표시함.
- 탄약이 0 이하일 때도 계속 발사되던 버그 수정.
- `BossShootingEditor Win64 Development` 빌드 성공.

**남은 BP 셋업/검증**:
- `Content/Game/Blueprints/Weapon/`에 `BP_Shotgun` 생성 또는 `BP_Rifle` 복제.
- 부모는 `BaseWeapon`.
- 권장 튜닝:
  - Damage: 8
  - FireRate: 0.8~1.0
  - MaxAmmo: 6
  - Range: 2500~3000
  - PelletCount: 8
  - SpreadAngleDegrees: 6~10
- 임시 테스트는 `BP_BaseCharacter`의 Default Weapon Class를 `BP_Shotgun`으로 바꿔서 가능.
- PIE Listen Server에서 한 번 쏠 때 여러 디버그 라인이 퍼지고, 서버에서 pellet hit 로그가 찍히는지 확인.

### Step 6 이후 (GDD §8 일정 참고)
### Step 6b 진행 중 — 유탄 projectile 기반

**완료된 코드 작업**:
- `ABaseProjectile` C++ 클래스 추가.
- Projectile은 `bReplicates=true`, `SetReplicateMovement(true)`로 서버 이동 결과를 클라이언트에 복제.
- `USphereComponent` 충돌 + `UProjectileMovementComponent` 이동 사용.
- 서버에서 충돌 시 `ApplyRadialDamage`로 방사 피해 처리.
- 폭발 위치/반경은 `NetMulticast`로 모든 클라이언트에 디버그 구 표시.
- `ABaseWeapon`에 `EWeaponFireMode` 추가:
  - `HitScan`
  - `Projectile`
- `ABaseWeapon`에 `ProjectileClass`, `ProjectileSpawnForwardOffset` 추가.
- `FireMode=Projectile`이면 서버에서 projectile actor를 spawn하고 발사 방향을 초기화.
- 기존 라이플/샷건 기본값은 `HitScan`이라 기존 동작 유지.
- `BossShootingEditor Win64 Development` 빌드 성공.

**남은 BP 셋업/검증**:
- `BP_GrenadeProjectile` 생성: 부모 `BaseProjectile`
- `BP_GrenadeLauncher` 생성 또는 `BP_Rifle` 복제 후 부모 `BaseWeapon` 유지
- `BP_GrenadeLauncher` 권장 튜닝:
  - FireMode: Projectile
  - ProjectileClass: BP_GrenadeProjectile
  - Damage/FireRate/MaxAmmo는 무기 쪽 탄약/발사 간격용으로 조정
  - MaxAmmo: 6
  - FireRate: 0.8~1.0
- `BP_GrenadeProjectile` 권장 튜닝:
  - Damage: 60
  - DamageRadius: 250
  - InitialSpeed: 1600
  - LifeSeconds: 5
- PIE Listen Server에서 projectile이 서버에서 spawn되고, 양쪽 화면에 이동/폭발 디버그가 보이는지 확인.
- 폭발 반경 안의 러셔/플레이어 체력이 서버에서 감소하고 복제되는지 확인.

### Step 6 이후 남은 작업
### Step 6c 진행 중 — 무기 픽업 + 1슬롯 교체 기반

**완료된 코드 작업**:
- `AWeaponPickup` C++ 클래스 추가.
- Pickup actor는 `bReplicates=true`.
- `USphereComponent` overlap으로 Pawn 감지.
- 서버에서만 overlap 처리.
- 서버가 `WeaponClass`를 spawn하고 `ABaseCharacter::EquipWeapon` 호출.
- pickup claim 후 actor destroy.
- `ABaseCharacter::EquipWeapon`은 1슬롯 교체 모델에 맞춰 기존 무기를 detach 후 destroy.
- `BossShootingEditor Win64 Development` 빌드 성공.

**남은 BP 셋업/검증**:
- `BP_WeaponPickup` 생성: 부모 `WeaponPickup`
- `WeaponClass`를 `BP_Rifle`, `BP_Shotgun`, `BP_GrenadeLauncher` 중 하나로 지정
- PickupMesh는 임시 cube/static mesh로 지정 가능
- 맵에 pickup 배치
- PIE Listen Server에서 host/remote client가 pickup을 먹었을 때:
  - 서버 로그에 pickup 로그가 찍히는지
  - 기존 무기가 제거되고 새 무기가 장착되는지
  - 다른 화면에서도 무기 교체 결과가 보이는지
  - pickup actor가 모든 화면에서 사라지는지 확인

### Step 6 이후 남은 작업
- 잡몹 스피터 (원거리 산성 침)
- 잡몹 웨이브 트리거
- 보스 (외계인 여왕) 3페이즈
- GameMode/GameState, ServerTravel
- UMG (메인 메뉴, 로비, 임무 보드, HUD, 보스 체력바)
- 부활 시스템

---

## 8. 디자인 결정 이력 (요약)

발표 시 "왜 이 선택?"에 답할 수 있게 정리된 결정들. GDD §10에 상세.

1. **GAS 채택 X** — 학습 비용 폭증, 별도 학습 프로젝트로 분리
2. **외계인 호러 (감염체 + 토착 변이체 혼합형)** — 인간/외계 메쉬 두 라인 활용
3. **여왕형 보스 3페이즈 (본체→소환→격노)** — 페이즈마다 다른 학습 메카닉
4. **`ACharacter` 베이스** (Pawn 아님) — CMC의 클라 예측·보정 무료 사용
5. **무기는 컴포넌트 X, 별도 액터** — 픽업/드롭/스왑 자연스러움, Lyra 스타일
6. **마우스 조준에 `DeprojectMousePositionToWorld` + 평면 교차** — 콜리전 의존 회피
7. **회전 동기화는 `SetActorRotation` + `bReplicateMovement`** — Replicated Yaw 대신 자동 동기화
8. **사망 처리 두 갈래 분리** — `HandleDeath` (게임 상태) + `Multicast_OnDeath` (연출)
9. **분대원 4명 lore 정체성 + 게임 메카닉은 동일** — 학습 키트 스코프 보호 + 확장 hook
10. **스테이지 선택 → "임무 보드"** — 단순 화면이 아닌 사령부 지령 흐름
11. **결과 화면 → 호스트 로비 복귀** — 분대 유지 + 재출격, PvE 코옵 표준
12. **★ BTTask BP → C++ 전환** — BP 노드 그래프가 사용자 학습 부담 크고, C++ Task는 추적/디버깅도 깔끔. 학습 키트 정신에도 부합

---

## 9. 학습 산출물 체크리스트 (현황)

GDD §9 학습 산출물:

- [x] `HasAuthority()` / `GetLocalRole()`로 권위 분기
- [x] `Replicated` 변수 (`DOREPLIFETIME`)
- [x] `RepNotify` 자동/수동 호출 둘 다
- [x] `Server` RPC (Reliable / Unreliable 둘 다)
- [x] `NetMulticast` RPC (Unreliable)
- [x] `CharacterMovementComponent` 클라 예측
- [x] `bReplicates`, `bReplicateMovement`
- [x] `SetOwner`로 RPC 라우팅 확보
- [x] `AttachToComponent` 시각 정보 RepNotify 보강 패턴
- [x] **C++ BTTaskNode 작성** (Codex 작업으로 추가 항목)
- [x] **AI 서버 권위 + 결과만 리플리케이트** (Step 5b 추격 동작 검증 완료, 로그 증거는 선택 과제)
- [ ] `GameMode` vs `GameState` 책임 분리
- [ ] `ServerTravel`로 레벨 이동
- [ ] 2개 이상 인스턴스 멀티 테스트 + 동기화 디버깅

---

**이 문서는 슬림 핸드오프 v2. Step 5b 추격 검증까지 반영. 새 AI는 §7의 Step 5c(러셔 근접 공격)부터 이어가면 됨.**
