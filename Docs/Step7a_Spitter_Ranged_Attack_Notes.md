# Step 7a 스피터 원거리 산성 침 구현 노트

> 목적: 러셔의 근접 공격과 다른 패턴인 **서버 타이머 기반 원거리 projectile 공격**을 학습한다.

이 문서는 2026-05-14 기준 Step 7a 코드 변경을 나중에 다시 공부하기 쉽게 풀어쓴 기록이다.

---

## 1. 이번에 만든 기능

`AAlienSpitter` C++ 클래스를 추가했다.

스피터는 러셔처럼 플레이어를 추격할 수 있지만, 공격 방식은 다르다.

1. 서버에서 일정 간격으로 살아있는 플레이어를 찾는다.
2. 공격 범위 안에 플레이어가 있으면 쿨다운을 확인한다.
3. 서버가 `AcidProjectileClass` projectile actor를 spawn한다.
4. projectile이 서버에서 이동/충돌/폭발/데미지를 처리한다.
5. 클라이언트는 replicated projectile movement와 health replication 결과를 본다.

---

## 2. 왜 기존 `BaseProjectile`을 재사용했나

유탄과 산성 침은 외형/수치가 다르지만 네트워크 패턴은 같다.

| 공통점 | 의미 |
|---|---|
| 서버에서 projectile spawn | 클라가 projectile 위치/충돌을 조작하지 못하게 함 |
| `SetReplicateMovement(true)` | 클라가 날아가는 actor를 볼 수 있음 |
| 서버 충돌 판정 | 맞았는지 서버가 결정 |
| 서버 damage 처리 | 체력 변화는 replicated health로 동기화 |
| multicast debug FX | 폭발/충돌 위치를 모두에게 보여주는 임시 연출 |

그래서 새 projectile C++ 클래스를 바로 만들기보다, `ABaseProjectile`을 BP 튜닝으로 재사용하는 쪽이 지금 단계에 맞다.

예:

- `BP_GrenadeProjectile`: 느리고 큰 폭발
- `BP_AcidProjectile`: 더 빠르거나 작고 약한 산성 침

---

## 3. 변경 파일 요약

### `AlienSpitter.h/cpp`

새 C++ 클래스:

```cpp
class AAlienSpitter : public AAlienBase
```

주요 튜닝값:

| 값 | 기본값 | 의미 |
|---|---:|---|
| `bCanRangedAttack` | true | 원거리 공격 사용 여부 |
| `AcidProjectileClass` | BP에서 지정 | 서버가 spawn할 산성 침 projectile |
| `RangedAttackRange` | 1200 | 공격 대상 탐색 범위 |
| `RangedAttackCooldown` | 2.0 | 발사 쿨다운 |
| `RangedAttackCheckInterval` | 0.25 | 서버 타이머 검사 주기 |
| `ProjectileSpawnForwardOffset` | 80 | 몸 중심보다 앞에서 발사 |
| `ProjectileSpawnUpOffset` | 60 | 바닥보다 위에서 발사 |

스피터 생성자에서:

```cpp
bCanMeleeAttack = false;
MaxHealth = 50.0f;
GetCharacterMovement()->MaxWalkSpeed = 260.0f;
```

의미:

- 스피터는 러셔처럼 붙어서 때리는 적이 아니므로 근접 공격 타이머를 꺼둔다.
- 러셔보다 체력을 조금 높게 잡았다.
- 원거리 적이라 이동 속도는 조금 낮췄다.

---

### `AlienBase.h`

러셔 근접 공격 관련 값들을 `private`에서 `protected`로 옮겼다.

이유:

- `AAlienSpitter`가 부모의 `bCanMeleeAttack` 기본값을 꺼야 한다.
- 이것은 외부 공개가 아니라 자식 클래스용 확장 포인트다.

---

### `BaseProjectile.h/cpp`

`InitProjectile`에 optional `IgnoredActor`를 추가했다.

```cpp
void InitProjectile(
    const FVector& LaunchDirection,
    AController* NewInstigatorController,
    AActor* NewDamageCauser,
    AActor* NewIgnoredActor = nullptr);
```

기존에는 `DamageCauser`만 ignore했다.

하지만 무기 발사에서는:

- `DamageCauser`: weapon actor
- 실제로 충돌을 무시해야 하는 대상: owning character

스피터 발사에서는:

- `DamageCauser`: spitter
- ignore 대상: spitter

이 둘을 분리하면 projectile이 발사자와 바로 충돌하거나, radial damage에 발사자가 포함되는 문제를 줄일 수 있다.

추가로 `BeginPlay`에서 BP의 `InitialSpeed` 값을 `ProjectileMovement`에 다시 반영하게 했다.

이유:

- C++ 생성자에서는 BP override 값이 아직 확정되기 전이다.
- `BP_AcidProjectile`에서 `InitialSpeed`를 바꿨는데 movement component의 `MaxSpeed`가 예전 값이면 속도 튜닝이 헷갈릴 수 있다.
- 그래서 런타임 시작 시점에 `ProjectileMovement->InitialSpeed`, `MaxSpeed`를 BP 값으로 맞춘다.

---

## 4. 네트워크 관점 실행 흐름

```text
Server:
  AAlienSpitter::BeginPlay
  -> StartRangedAttackTimer_ServerOnly
  -> TryRangedAttack_ServerOnly 반복
  -> 살아있는 가장 가까운 플레이어 검색
  -> cooldown 확인
  -> AcidProjectileClass spawn
  -> Projectile->InitProjectile(...)

Projectile on server:
  movement authority
  -> collision hit
  -> ApplyRadialDamage
  -> Multicast_Explode
  -> Destroy

Clients:
  projectile movement replication 관찰
  -> explosion debug sphere 관찰
  -> health replication으로 데미지 결과 관찰
```

핵심:

> 스피터는 “클라 화면에서 projectile이 보이는 적”이지만, 발사 판단과 데미지 결과는 서버가 결정한다.

---

## 5. BP 셋업 방법

### `BP_AcidProjectile`

추천 경로:

```text
Content/Game/Blueprints/Enemy/BP_AcidProjectile
```

또는 projectile을 무기/적 공용으로 모으고 싶다면:

```text
Content/Game/Blueprints/Projectile/BP_AcidProjectile
```

부모 클래스:

```text
BaseProjectile
```

권장 시작값:

| 항목 | 값 |
|---|---:|
| Damage | 15 |
| DamageRadius | 120 |
| InitialSpeed | 1200 |
| LifeSeconds | 4 |

### `BP_AlienSpitter`

추천 경로:

```text
Content/Game/Blueprints/Enemy/BP_AlienSpitter
```

부모 클래스:

```text
AlienSpitter
```

Class Defaults에서:

| 항목 | 값 |
|---|---|
| AcidProjectileClass | `BP_AcidProjectile` |
| RangedAttackRange | 1200 |
| RangedAttackCooldown | 2.0 |
| RangedAttackCheckInterval | 0.25 |

AI Controller는 러셔와 같은 `BP_AlienAIController`를 임시로 써도 된다.

이 경우:

- Behavior Tree는 계속 가까운 플레이어로 이동한다.
- 공격은 `AAlienSpitter` 서버 타이머가 별도로 처리한다.

나중에 “거리 유지”가 필요해지면 스피터 전용 Behavior Tree를 만든다.

---

## 6. 테스트 체크리스트

- [ ] `BP_AcidProjectile` 부모가 `BaseProjectile`이다.
- [ ] `BP_AlienSpitter` 부모가 `AlienSpitter`이다.
- [ ] `BP_AlienSpitter.AcidProjectileClass`에 `BP_AcidProjectile`이 들어 있다.
- [ ] `BP_AlienSpitter`의 AI Controller Class가 `BP_AlienAIController`로 되어 있다.
- [ ] 맵에 배치하면 플레이어 쪽으로 이동한다.
- [ ] 범위 안에 들어오면 서버 로그에 산성 침 발사 로그가 찍힌다.
- [ ] projectile이 host/remote client 화면 모두에서 보인다.
- [ ] projectile 충돌/폭발 디버그 구가 양쪽 화면에 보인다.
- [ ] 맞은 플레이어 체력이 서버에서 감소하고 클라에 복제된다.

---

## 7. 현재 한계

아직 없는 것:

- 스피터 전용 Behavior Tree
- 거리 유지/후퇴 AI
- line of sight 검사
- 공격 애니메이션
- 산성 침 전용 mesh/VFX/SFX
- projectile arc/gravity 튜닝
- 산성 장판 DOT
- 플레이어별 어그로/타겟 우선순위

현재는 “원거리 적도 서버가 projectile 공격을 결정한다”는 네트워크 학습용 최소 버전이다.

---

## 8. 발표/회고 때 말할 수 있는 학습 포인트

- 러셔는 서버 타이머로 근접 데미지를 직접 넣는 적이다.
- 스피터는 서버 타이머로 projectile actor를 spawn하는 적이다.
- 둘 다 공격 판단은 서버에서 하지만, 클라이언트가 관찰하는 결과는 다르다.
- projectile은 replicated movement로 날아가는 모습이 보이고, 충돌/데미지는 서버에서만 확정된다.
- `DamageCauser`와 `IgnoredActor`를 분리해서 weapon projectile과 enemy projectile 모두 안전하게 재사용할 수 있게 했다.

짧게 정리하면:

> Step 7a는 “서버 권위 AI 공격”을 근접 데미지에서 원거리 projectile spawn 패턴으로 확장한 단계다.
