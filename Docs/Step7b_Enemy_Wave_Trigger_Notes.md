# Step 7b 잡몹 웨이브 트리거 구현 노트

> 목적: 플레이어가 특정 구역에 들어오면 **서버가 적 웨이브를 스폰**하는 흐름을 학습한다.

이 문서는 2026-05-14 기준 Step 7b 코드 변경을 나중에 다시 공부하기 쉽게 풀어쓴 기록이다.

---

## 1. 이번에 만든 기능

`AEnemyWaveTrigger` C++ 클래스를 추가했다.

이 Actor를 맵에 배치하고 BP에서 적 목록을 설정하면, 플레이어가 trigger box에 들어왔을 때 서버가 적들을 spawn한다.

흐름:

1. 맵에 `BP_EnemyWaveTrigger`를 배치한다.
2. `EnemyClasses` 배열에 `BP_AlienRusher`, `BP_AlienSpitter` 같은 적 BP를 넣는다.
3. 필요하면 `SpawnPoints` 배열에 `TargetPoint` actor들을 넣는다.
4. 플레이어가 trigger box에 overlap한다.
5. 서버가 살아있는 플레이어인지 확인한다.
6. 서버가 한 번만 wave를 시작하고 적들을 spawn한다.
7. spawn된 적들은 replicated actor/movement로 클라이언트에도 보인다.

---

## 2. 왜 서버에서만 wave를 시작해야 하나

웨이브 스폰은 “게임 상태를 바꾸는 일”이다.

클라이언트가 각자 spawn하면:

- 호스트 화면과 클라 화면의 적 수가 달라질 수 있음
- 클라가 마음대로 웨이브를 시작할 수 있음
- 적 AI/데미지/사망 흐름이 서버 권위 모델과 어긋남

그래서 overlap은 서버에서만 실제 wave 시작으로 이어진다.

| 머신 | 하는 일 |
|---|---|
| server | overlap 판정, 중복 실행 방지, enemy spawn |
| owning client | trigger를 밟지만 spawn을 결정하지 않음 |
| remote clients | 서버가 spawn한 적 actor와 movement 결과를 봄 |

핵심 문장:

> 웨이브는 클라가 “시작했다”고 주장하는 게 아니라, 서버가 trigger overlap을 보고 spawn 결과를 만든다.

---

## 3. 변경 파일 요약

### `EnemyWaveTrigger.h/cpp`

새 C++ 클래스:

```cpp
class AEnemyWaveTrigger : public AActor
```

주요 컴포넌트:

```cpp
UBoxComponent* TriggerBox;
```

주요 설정값:

| 값 | 의미 |
|---|---|
| `EnemyClasses` | 서버가 wave 시작 시 spawn할 적 BP 목록 |
| `SpawnPoints` | 적이 등장할 위치 목록 |
| `SpawnScatterRadius` | 같은 위치에 겹치지 않게 XY 랜덤 오프셋을 주는 거리 |
| `bTriggered` | 서버 내부 중복 실행 방지 |

`bTriggered`는 replicate하지 않았다.

이유:

- 클라이언트 UI가 읽는 값이 아니다.
- 서버가 “이미 이 trigger를 썼는가”만 알면 된다.
- 클라이언트에게 중요한 결과는 spawn된 적 actor들이 보이는 것이다.

---

## 4. 실행 흐름

### 1. 생성자

```cpp
bReplicates = true;
TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
```

trigger actor 자체는 맵에 놓이는 replicated actor로 둔다.

실제 핵심은 `TriggerBox`다.

```cpp
TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
```

Pawn만 overlap 대상으로 잡는다.

---

### 2. BeginPlay

```cpp
if (HasAuthority())
{
    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AEnemyWaveTrigger::OnTriggerOverlap);
}
```

overlap callback을 서버에서만 바인딩한다.

클라이언트 overlap은 enemy spawn으로 이어지지 않는다.

---

### 3. Overlap 처리

```cpp
if (!HasAuthority() || bTriggered)
{
    return;
}
```

서버가 아니거나 이미 실행된 trigger면 무시한다.

```cpp
ABaseCharacter* Character = Cast<ABaseCharacter>(OtherActor);
if (!Character || Character->IsDead())
{
    return;
}
```

살아있는 플레이어 캐릭터만 wave를 시작할 수 있다.

---

### 4. SpawnWave

```cpp
for (int32 Index = 0; Index < EnemyClasses.Num(); ++Index)
{
    ...
    GetWorld()->SpawnActor<AAlienBase>(EnemyClass, SpawnTransform, Params);
}
```

`EnemyClasses` 배열에 들어 있는 BP를 순서대로 spawn한다.

예:

| Index | EnemyClass |
|---:|---|
| 0 | `BP_AlienRusher` |
| 1 | `BP_AlienRusher` |
| 2 | `BP_AlienSpitter` |

그러면 trigger 1개가 러셔 2마리 + 스피터 1마리를 spawn할 수 있다.

---

## 5. BP 셋업 방법

### `BP_EnemyWaveTrigger`

추천 경로:

```text
Content/Game/Blueprints/Enemy/BP_EnemyWaveTrigger
```

부모 클래스:

```text
EnemyWaveTrigger
```

설정:

| 항목 | 예시 |
|---|---|
| EnemyClasses[0] | `BP_AlienRusher` |
| EnemyClasses[1] | `BP_AlienRusher` |
| EnemyClasses[2] | `BP_AlienSpitter` |
| SpawnScatterRadius | 120 |

`SpawnPoints`는 비워도 된다.

비우면 trigger actor 위치 주변에 적들이 나온다.

더 깔끔하게 하려면:

1. 맵에 `TargetPoint` 2~3개 배치
2. `BP_EnemyWaveTrigger.SpawnPoints` 배열에 넣기
3. 적들이 해당 위치 기준으로 spawn됨

---

## 6. 테스트 체크리스트

- [ ] `BP_EnemyWaveTrigger` 부모가 `EnemyWaveTrigger`다.
- [ ] `EnemyClasses` 배열에 적 BP가 하나 이상 들어 있다.
- [ ] TriggerBox 크기가 플레이어가 지나갈 길을 덮는다.
- [ ] PIE Listen Server에서 host가 밟으면 적들이 spawn된다.
- [ ] remote client가 밟아도 서버 로그에 wave 시작 로그가 찍힌다.
- [ ] spawn된 적들이 host/remote client 화면 모두에서 보인다.
- [ ] trigger를 여러 번 밟아도 wave가 한 번만 나온다.

---

## 7. 현재 한계

아직 없는 것:

- wave 완료 조건
- 다음 wave 자동 시작
- 남은 적 수 카운트
- GameMode/GameState와 연결
- trigger 비활성화/시각화
- spawn delay
- 난이도별 spawn table

현재는 “구역 진입 → 서버가 한 번 적 목록 spawn”까지의 최소 버전이다.

---

## 8. 발표/회고 때 말할 수 있는 학습 포인트

- 플레이어 overlap 자체는 단순한 이벤트지만, wave spawn은 게임 상태 변경이라 서버 권위로 처리했다.
- `bTriggered`는 서버 내부 guard라 replicate하지 않았다.
- 클라이언트는 wave 시작을 결정하지 않고, 서버가 spawn한 enemy actor들을 replication으로 본다.
- BP 배열로 enemy class를 넣게 해서 러셔/스피터 조합을 C++ 수정 없이 바꿀 수 있게 했다.

짧게 정리하면:

> Step 7b는 “맵 이벤트가 서버에서 gameplay actor spawn으로 이어지는 구조”를 만든 단계다.
