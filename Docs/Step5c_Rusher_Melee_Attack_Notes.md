# Step 5c 러셔 근접 공격 구현 노트

> 목적: “러셔가 플레이어를 따라간다”에서 한 단계 더 나아가, **서버가 공격 가능 여부를 판단하고 서버가 플레이어 체력을 깎는 흐름**을 학습한다.

이 문서는 2026-05-14 기준 Step 5c 코드 변경을 나중에 다시 공부하기 쉽게 풀어쓴 기록이다.

---

## 1. 이번에 만든 기능

러셔가 플레이어에게 가까이 붙으면 1초마다 10 데미지를 준다.

현재 구현은 공격 애니메이션이나 Behavior Tree 공격 Task까지는 가지 않은 **최소 동작 버전**이다.

동작 흐름:

1. 러셔가 서버에서만 근접 공격 체크 타이머를 켠다.
2. 0.2초마다 주변 플레이어를 검사한다.
3. 공격 범위 안에 살아있는 플레이어가 있으면 가장 가까운 플레이어를 고른다.
4. 마지막 공격 이후 1초가 지났으면 서버에서 `TakeDamage`를 호출한다.
5. 플레이어의 `CurrentHealth`가 서버에서 줄고, 기존 RepNotify 흐름으로 클라이언트에 복제된다.
6. 러셔가 죽으면 공격 타이머를 끈다.

---

## 2. 왜 서버에서만 처리해야 하나

이 프로젝트는 4인 listen-server 멀티플레이 게임이다.

러셔 공격은 체력을 깎는 gameplay result이므로 클라이언트가 판단하면 안 된다.

클라이언트가 직접 “내가 맞았다/안 맞았다”를 결정하면 생기는 문제:

- 클라이언트마다 거리 판정 결과가 다를 수 있음
- 렉이나 보간 때문에 화면상 위치가 서로 다를 수 있음
- 나쁜 클라이언트가 데미지를 무시할 수 있음
- 호스트에서는 되는 것처럼 보이는데 remote client에서 깨질 수 있음

그래서 이번 구조는 이렇게 나눈다.

| 머신 | 하는 일 |
|---|---|
| 서버 / listen-server host | 공격 범위 검사, 쿨다운 검사, 데미지 적용 |
| remote client | 서버가 복제해 준 체력/이동 결과를 봄 |
| 모든 클라이언트 | 나중에 공격 애니메이션/VFX/SFX를 재생할 수 있음 |

핵심 문장:

> AI가 만든 결과도 gameplay state를 바꾸면 서버 권위여야 한다.

---

## 3. 변경 파일 요약

### `BaseCharacter.h`

추가:

```cpp
UFUNCTION(BlueprintPure, Category = "Character|State")
bool IsDead() const { return CurrentHealth <= 0.0f; }
```

용도:

- 러셔가 이미 죽은 플레이어를 공격 대상으로 고르지 않게 하기 위한 조회 함수.
- `CurrentHealth`는 protected라서 `AAlienBase`가 직접 볼 수 없다.
- 그래서 public 함수로 “죽었는지만” 안전하게 공개했다.

백엔드식으로 보면:

- `CurrentHealth` 필드는 private/protected로 숨겨두고,
- 외부에는 `isDead()` 같은 읽기 전용 메서드만 열어둔 느낌이다.

---

### `AlienBase.h`

추가된 튜닝값:

```cpp
UPROPERTY(EditDefaultsOnly, Category = "Alien|Melee")
bool bCanMeleeAttack = true;

UPROPERTY(EditDefaultsOnly, Category = "Alien|Melee", meta = (EditCondition = "bCanMeleeAttack", ClampMin = "0.0"))
float MeleeAttackRange = 120.0f;

UPROPERTY(EditDefaultsOnly, Category = "Alien|Melee", meta = (EditCondition = "bCanMeleeAttack", ClampMin = "0.0"))
float MeleeAttackDamage = 10.0f;

UPROPERTY(EditDefaultsOnly, Category = "Alien|Melee", meta = (EditCondition = "bCanMeleeAttack", ClampMin = "0.0"))
float MeleeAttackCooldown = 1.0f;

UPROPERTY(EditDefaultsOnly, Category = "Alien|Melee", meta = (EditCondition = "bCanMeleeAttack", ClampMin = "0.05"))
float MeleeAttackCheckInterval = 0.2f;
```

의미:

| 값 | 의미 |
|---|---|
| `bCanMeleeAttack` | 이 적이 근접 공격을 할 수 있는지 |
| `MeleeAttackRange` | 공격 가능 거리 |
| `MeleeAttackDamage` | 한 번 공격할 때 데미지 |
| `MeleeAttackCooldown` | 실제 공격 간격 |
| `MeleeAttackCheckInterval` | 공격 가능한지 검사하는 주기 |

왜 `EditDefaultsOnly`?

- 러셔 BP에서 값을 조정할 수 있게 하기 위해.
- 하지만 런타임 중 아무나 바꾸는 값은 아니므로 기본값 튜닝 용도에 맞게 `EditDefaultsOnly`로 둔다.

왜 `MeleeAttackCheckInterval`과 `MeleeAttackCooldown`을 분리했나?

- `CheckInterval`: “자주 확인할까?”
- `Cooldown`: “실제로 몇 초마다 때릴까?”

예를 들어 0.2초마다 거리 체크를 해도, 공격은 1초마다 한 번만 나간다.

---

### `AlienBase.cpp`

#### 1. `BeginPlay`

```cpp
if (HasAuthority())
{
    CurrentHealth = MaxHealth;
    StartMeleeAttackTimer_ServerOnly();
}
```

중요한 점:

- `HasAuthority()` 안에서만 타이머를 켠다.
- 즉 클라이언트는 공격 체크 타이머를 돌리지 않는다.
- remote client 화면에 러셔가 보여도, 그 러셔가 클라에서 데미지를 판단하지 않는다.

---

#### 2. `StartMeleeAttackTimer_ServerOnly`

```cpp
GetWorldTimerManager().SetTimer(
    MeleeAttackTimerHandle,
    this,
    &AAlienBase::TryMeleeAttack_ServerOnly,
    MeleeAttackCheckInterval,
    true);
```

의미:

- Tick을 켜지 않고 타이머로 검사한다.
- 러셔마다 0.2초마다 `TryMeleeAttack_ServerOnly`를 실행한다.

왜 Tick이 아니라 Timer?

- 공격 판정은 매 프레임 필요하지 않다.
- 60fps 기준 Tick은 초당 60회지만, 지금은 초당 5회 체크면 충분하다.
- “계속 감시해야 하지만 매 프레임은 아닌 로직”에 Timer가 잘 맞는다.

---

#### 3. `TryMeleeAttack_ServerOnly`

핵심 구조:

```cpp
if (!HasAuthority() || IsDead())
{
    return;
}

const float CurrentTime = World->GetTimeSeconds();
if (CurrentTime - LastMeleeAttackTime < MeleeAttackCooldown)
{
    return;
}

ABaseCharacter* Target = FindClosestMeleeTarget_ServerOnly();
if (!Target)
{
    return;
}

LastMeleeAttackTime = CurrentTime;
Target->TakeDamage(MeleeAttackDamage, DamageEvent, GetController(), this);
```

체크 순서:

1. 서버가 아니면 return
2. 러셔가 죽었으면 return
3. 쿨다운이 안 끝났으면 return
4. 범위 안 타겟이 없으면 return
5. 조건을 모두 통과하면 데미지 적용

이런 식으로 조건을 앞에서 하나씩 걸러내는 방식을 guard clause라고 부른다.

장점:

- 들여쓰기가 깊어지지 않는다.
- “왜 공격이 안 나가는지” 조건을 위에서부터 읽기 쉽다.

---

#### 4. `FindClosestMeleeTarget_ServerOnly`

역할:

- 월드 안의 모든 `ABaseCharacter`를 훑는다.
- 죽은 플레이어는 제외한다.
- `MeleeAttackRange` 안에 있는 플레이어 중 가장 가까운 대상을 반환한다.

```cpp
for (TActorIterator<ABaseCharacter> It(World); It; ++It)
{
    ABaseCharacter* Candidate = *It;
    if (!IsValid(Candidate) || Candidate->IsDead())
    {
        continue;
    }

    const float DistanceSq = FVector::DistSquared2D(AlienLocation, Candidate->GetActorLocation());
    if (DistanceSq <= ClosestDistanceSq)
    {
        ClosestDistanceSq = DistanceSq;
        ClosestTarget = Candidate;
    }
}
```

왜 `DistSquared2D`?

- `Dist`는 제곱근 계산이 들어간다.
- 거리 비교만 할 때는 제곱거리로 비교해도 결과가 같다.
- 탑다운 게임이라 높이 차이는 일단 무시하고 XY 거리만 본다.

왜 `TActorIterator`?

- 지금 단계에서는 플레이어 수가 최대 4명이라 단순하고 충분하다.
- 나중에 플레이어 목록을 `GameState`나 캐시된 배열로 관리하면 더 깔끔해질 수 있다.
- 지금은 학습용으로 “월드에서 플레이어 찾기” 흐름을 눈으로 보기 쉽다.

---

#### 5. `HandleDeath`

추가:

```cpp
StopMeleeAttackTimer_ServerOnly();
```

이유:

- 죽은 러셔가 계속 공격 체크를 하면 안 된다.
- `SetLifeSpan(3.0f)`로 3초 동안 시체가 남아있는 동안에도 타이머가 돌 수 있으니 명시적으로 끈다.

---

## 4. 왜 Behavior Tree Task로 바로 안 만들었나

원래 더 정석적인 AI 구조는 이런 식이다.

```text
Root
 └─ Selector
     ├─ Sequence: 공격 가능?
     │   ├─ Is Target In Range
     │   └─ Attack Target
     └─ Sequence: 추격
         ├─ Find Closest Player
         └─ Move To TargetActor
```

하지만 지금은 Step 5c 첫 구현이기 때문에 일부러 단순하게 갔다.

현재 방식:

- Behavior Tree는 계속 추격 담당
- `AAlienBase` 서버 타이머가 근접 데미지 담당

장점:

- BT 에셋을 바로 복잡하게 바꾸지 않아도 된다.
- 서버 권위 데미지 흐름을 먼저 검증할 수 있다.
- “데미지가 줄어드는가?”라는 핵심 학습 목표를 빠르게 확인할 수 있다.

단점:

- 공격 행동이 BT의 의사결정 트리에 명시적으로 보이지 않는다.
- 공격 애니메이션과 이동 정지 같은 세밀한 제어가 부족하다.
- 나중에 스피터/보스처럼 행동 종류가 늘어나면 BT Task로 분리하는 편이 좋다.

다음 리팩토링 후보:

- `UBTTask_MeleeAttack`
- `UBTDecorator_IsTargetInMeleeRange`
- `AAlienBase::CanMeleeAttack`
- `AAlienBase::ServerPerformMeleeAttack` 또는 서버 전용 일반 함수

---

## 5. 네트워크 관점에서 보는 실행 흐름

러셔가 remote client 플레이어를 때리는 상황:

```text
서버:
  AAlienBase 타이머 실행
  → 범위 안 ABaseCharacter 찾음
  → Target->TakeDamage(10)
  → Target.CurrentHealth 감소
  → OnRep_Health 수동 호출
  → CurrentHealth replication 대기

remote client:
  서버에서 복제된 CurrentHealth 수신
  → OnRep_Health 자동 호출
  → 로그/HUD 갱신 가능
```

중요:

- 클라이언트는 `TakeDamage`를 직접 호출하지 않는다.
- 클라이언트는 러셔 공격 성공 여부를 결정하지 않는다.
- 클라이언트는 결과로 복제된 체력 값을 받는다.

---

## 6. 테스트 체크리스트

PIE 설정:

- Play As Listen Server
- Number of Players: 2 이상

확인할 것:

- [ ] 러셔가 플레이어에게 접근한다.
- [ ] 가까이 붙으면 서버 로그에 `근접 공격` 로그가 찍힌다.
- [ ] 플레이어 체력이 1초마다 10씩 줄어든다.
- [ ] 호스트 플레이어도 맞는다.
- [ ] remote client 플레이어도 맞는다.
- [ ] 양쪽 화면에서 체력 감소 결과가 같게 보인다.
- [ ] 러셔를 죽이면 더 이상 근접 공격 로그가 찍히지 않는다.
- [ ] 죽은 플레이어는 공격 대상으로 고르지 않는다. 현재 사망 연출은 아직 플레이어 쪽에 없으므로 추후 재확인 필요.

---

## 7. 지금 코드의 한계

현재 구현은 “학습용 최소 공격”이다.

아직 없는 것:

- 공격 애니메이션
- 공격할 때 이동 정지
- 공격 선딜/후딜
- 공격 판정 순간과 애니메이션 타이밍 동기화
- 클라이언트에 보이는 공격 이펙트
- Behavior Tree 안에서 공격 분기 표시
- 플레이어 사망/다운 상태 처리

그래도 이번 단계에서 얻은 핵심은 충분하다.

> AI가 서버에서 판단하고, 그 판단이 체력이라는 replicated gameplay state를 바꾼다.

---

## 8. 발표/회고 때 말할 수 있는 학습 포인트

- 러셔 추격은 Step 5b에서 서버 BT + replicated movement로 처리했다.
- Step 5c에서는 그 AI 결과가 실제 gameplay state, 즉 플레이어 체력에 영향을 주게 만들었다.
- 데미지 판정은 클라이언트가 아니라 서버에서만 수행했다.
- 체력은 기존 `ABaseCharacter::CurrentHealth` replication 흐름을 재사용했다.
- Tick 대신 Timer를 사용해 불필요한 매 프레임 체크를 피했다.
- 공격 튜닝값은 BP에서 조정 가능하게 `UPROPERTY(EditDefaultsOnly)`로 열었다.

짧게 정리하면:

> Step 5b가 “서버 AI가 움직임을 결정한다”였다면, Step 5c는 “서버 AI가 데미지 결과도 결정한다”이다.
