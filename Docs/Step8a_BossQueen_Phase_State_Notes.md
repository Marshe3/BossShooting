# Step 8a BossQueen 페이즈 상태 구현 노트

> 목적: 보스 체력 구간에 따라 **서버가 phase를 결정하고, 클라이언트는 replicated phase를 받는 구조**를 학습한다.

이 문서는 2026-05-14 기준 Step 8a 코드 변경을 나중에 다시 공부하기 쉽게 풀어쓴 기록이다.

---

## 1. 이번에 만든 기능

`ABossQueen` C++ 클래스를 추가했다.

아직 보스 공격 패턴은 없고, 먼저 보스전의 중심 상태인 phase 기반을 만들었다.

흐름:

1. 보스가 서버에서 데미지를 받는다.
2. 부모 `AAlienBase::TakeDamage`가 체력을 깎는다.
3. `ABossQueen`이 남은 HP 비율을 계산한다.
4. HP 60% 이하가 되면 Phase2로 전환한다.
5. HP 30% 이하가 되면 Phase3로 전환한다.
6. 죽으면 Dead phase로 전환한다.
7. `CurrentPhase`가 클라이언트에 replication되고 `OnRep_CurrentPhase`가 호출된다.

---

## 2. 왜 phase를 replicated property로 만들었나

보스 phase는 일시적인 연출만이 아니라 게임 상태다.

phase에 따라 앞으로 달라질 것들:

- 공격 패턴
- 이동 속도
- 무적 여부
- 소환 여부
- HUD 보스 phase 표시
- 보스전 연출

그래서 multicast RPC만으로 “페이즈 바뀌었다”를 한번 뿌리는 것보다, `CurrentPhase`를 replicated property로 들고 있는 편이 안전하다.

| 머신 | 하는 일 |
|---|---|
| server | HP 비율 계산, phase 결정, `CurrentPhase` 변경 |
| clients | replicated `CurrentPhase` 수신, `OnRep_CurrentPhase`로 HUD/연출 hook 실행 |

핵심 문장:

> 보스 phase는 서버가 결정하는 지속 상태이고, 클라이언트는 복제된 phase 값을 보고 같은 상태로 맞춘다.

---

## 3. 변경 파일 요약

### `BossQueen.h/cpp`

새 C++ 클래스:

```cpp
class ABossQueen : public AAlienBase
```

새 enum:

```cpp
enum class EBossQueenPhase : uint8
{
    Phase1,
    Phase2,
    Phase3,
    Dead
};
```

주요 replicated state:

```cpp
UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase)
EBossQueenPhase CurrentPhase;

UPROPERTY(Replicated)
bool bIsInvincible;
```

주요 튜닝값:

| 값 | 기본값 | 의미 |
|---|---:|---|
| `MaxHealth` | 600 | 보스 체력 |
| `Phase2HealthRatio` | 0.6 | HP 60% 이하에서 Phase2 |
| `Phase3HealthRatio` | 0.3 | HP 30% 이하에서 Phase3 |

---

## 4. 실행 흐름

### 1. 생성자

```cpp
MaxHealth = 600.0f;
bCanMeleeAttack = false;
```

보스는 `AAlienBase`를 상속하지만 러셔식 근접 타이머는 쓰지 않는다.

이후 보스 전용 패턴을 따로 넣기 위해 꺼둔 상태다.

---

### 2. 데미지 처리

```cpp
const float ActualDamage = Super::TakeDamage(...);

if (ActualDamage > 0.0f && !IsDead())
{
    UpdatePhase_ServerOnly();
}
```

체력 감소는 부모가 처리한다.

보스 클래스는 감소한 체력 결과를 보고 phase를 판단한다.

이렇게 한 이유:

- `AAlienBase`의 Health/Death replication 흐름 재사용
- 보스는 “페이즈 판단”만 추가
- 공통 damage 코드를 중복하지 않음

---

### 3. phase 판단

```cpp
const float HealthRatio = MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
```

HP 비율에 따라:

| 조건 | Phase |
|---|---|
| `HealthRatio <= 0.3` | Phase3 |
| `HealthRatio <= 0.6` | Phase2 |
| 그 외 | Phase1 |

---

### 4. RepNotify

```cpp
CurrentPhase = NewPhase;
OnRep_CurrentPhase(OldPhase);
```

서버에서 phase 값을 바꾼 뒤 `OnRep_CurrentPhase`를 직접 호출한다.

이유:

- 클라이언트는 replication으로 `OnRep`가 자동 호출된다.
- 서버는 값만 바꿔서는 `OnRep`가 자동 호출되지 않는다.
- 서버도 같은 로그/연출 hook을 쓰려면 수동 호출이 필요하다.

---

## 5. BP 셋업 방법

### `BP_BossQueen`

추천 경로:

```text
Content/Game/Blueprints/Enemy/BP_BossQueen
```

부모 클래스:

```text
BossQueen
```

권장 확인:

| 항목 | 값 |
|---|---|
| MaxHealth | 600 |
| Phase2HealthRatio | 0.6 |
| Phase3HealthRatio | 0.3 |
| AI Controller Class | 일단 비워도 됨 또는 추후 보스 전용 AIController |

임시 검증은 맵에 배치하고 플레이어 무기로 때려서 Output Log를 보면 된다.

---

## 6. 테스트 체크리스트

- [ ] `BP_BossQueen` 부모가 `BossQueen`이다.
- [ ] 보스를 맵에 배치한다.
- [ ] 플레이어가 보스를 공격하면 서버 로그에 체력 감소가 찍힌다.
- [ ] HP 60% 이하에서 Phase1 → Phase2 로그가 찍힌다.
- [ ] HP 30% 이하에서 Phase2 → Phase3 로그가 찍힌다.
- [ ] 사망 시 Dead phase 로그가 찍힌다.
- [ ] Listen Server remote client 화면에서도 같은 phase 전환 결과를 확인한다.

---

## 7. 현재 한계

아직 없는 것:

- 보스 전용 Behavior Tree
- Phase별 공격 패턴
- Phase2 소환/무적 루프
- Phase3 이동 속도 증가
- 보스 체력바 HUD
- 보스 사망 승리 처리

현재는 “보스 phase replicated state”만 만든 최소 기반이다.

---

## 8. 발표/회고 때 말할 수 있는 학습 포인트

- 보스 phase는 서버가 결정하는 지속 상태라 replicated property로 만들었다.
- 체력 감소는 부모 `AAlienBase` 흐름을 재사용하고, 보스는 phase 판단만 추가했다.
- `RepNotify`는 클라이언트의 HUD/연출 hook이 될 수 있다.
- 서버는 RepNotify가 자동 호출되지 않으므로 같은 처리를 위해 수동 호출했다.

짧게 정리하면:

> Step 8a는 보스전을 시작하기 전에 “HP 기반 phase 상태를 서버가 결정하고 모든 클라이언트가 같은 phase를 보게 하는 뼈대”를 만든 단계다.
