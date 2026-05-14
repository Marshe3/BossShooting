# Step 6a 샷건 다발 히트스캔 구현 노트

> 목적: 기존 돌격소총 히트스캔 무기 코드를 확장해서, **한 번의 발사 요청으로 여러 pellet 라인트레이스가 나가는 샷건 기반**을 만든다.

이 문서는 2026-05-14 기준 Step 6a 코드 변경을 나중에 다시 공부하기 쉽게 풀어쓴 기록이다.

---

## 1. 이번에 만든 기능

`ABaseWeapon`이 이제 라이플과 샷건을 둘 다 표현할 수 있다.

기본값:

- `PelletCount = 1`
- `SpreadAngleDegrees = 0`

그래서 기존 `BP_Rifle`은 그대로 단일 라인트레이스 무기처럼 동작한다.

샷건 BP에서는 예를 들어 이렇게 바꾸면 된다.

| 값 | 예시 |
|---|---|
| `Damage` | 8 |
| `FireRate` | 0.8~1.0 |
| `MaxAmmo` | 6 |
| `Range` | 2500~3000 |
| `PelletCount` | 8 |
| `SpreadAngleDegrees` | 6~10 |

그러면 한 번 발사할 때 서버에서 8개의 방향을 만들고, 각 방향으로 라인트레이스를 쏜다.

---

## 2. 왜 `BaseWeapon`을 확장했나

현재 프로젝트의 무기 구조는 `ABaseWeapon` 하나를 BP로 튜닝해서 무기 차이를 만드는 방식이다.

그래서 샷건을 위해 바로 `AShotgunWeapon` C++ 클래스를 새로 만들기보다, 먼저 `ABaseWeapon`에 다음 개념을 추가했다.

- pellet 개수
- spread 각도

이렇게 하면:

- `BP_Rifle`: pellet 1개, spread 0도
- `BP_Shotgun`: pellet 8개, spread 6~10도

같은 C++ 흐름을 재사용할 수 있다.

나중에 샷건만 특수한 reload, recoil, animation, pellet falloff가 필요해지면 그때 별도 C++ 자식 클래스로 분리해도 된다.

---

## 3. 변경 파일 요약

### `BaseWeapon.h`

추가된 튜닝값:

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats", meta = (ClampMin = "1"))
int32 PelletCount = 1;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats", meta = (ClampMin = "0.0", Units = "Degrees"))
float SpreadAngleDegrees = 0.0f;
```

의미:

| 값 | 의미 |
|---|---|
| `PelletCount` | 한 번 발사할 때 라인트레이스를 몇 개 쏠지 |
| `SpreadAngleDegrees` | 정면 기준으로 pellet 방향이 퍼지는 각도 |

기본값을 라이플 기준으로 잡아둔 이유:

- 기존 `BP_Rifle`을 깨지 않기 위해.
- 새 값을 추가했지만 기본 상태에서는 이전과 같은 단일 히트스캔이다.

---

### RPC 시그니처 변경

기존:

```cpp
void Server_Fire(const FVector& TraceStart, const FVector& TraceEnd);
```

변경:

```cpp
void Server_Fire(const FVector& AimDirection);
```

이유:

- 기존 구조는 클라이언트가 TraceStart/TraceEnd를 만들어 서버에 넘겼다.
- 학습용으로는 괜찮지만, 네트워크 원칙상 클라이언트가 hit trace를 너무 많이 정하면 위험하다.
- 이번 변경에서는 클라이언트가 “나는 이 방향으로 쏘고 싶다”만 요청한다.
- 서버는 자기 기준의 `OwningCharacter` 위치에서 `TraceStart`를 다시 계산한다.

즉:

```text
클라: 이 방향으로 쏠래요
서버: 네 위치는 서버에서 아는 값으로 다시 계산하고, 실제 trace는 내가 할게
```

---

### Multicast FX 변경

기존:

```cpp
void Multicast_PlayFireFX(const FVector& TraceStart, const FVector& TraceEnd, AActor* HitActor);
```

변경:

```cpp
void Multicast_PlayFireFX(
    const FVector& TraceStart,
    const TArray<FVector>& TraceEnds,
    const TArray<AActor*>& HitActors);
```

이유:

- 라이플은 한 발에 trace 하나면 충분했다.
- 샷건은 한 번 발사에 trace가 여러 개다.
- pellet마다 multicast RPC를 여러 번 보내면 구조가 지저분하고 RPC 수가 늘어난다.
- 그래서 한 번의 multicast에 여러 trace end와 hit actor 배열을 담았다.

현재는 디버그 라인/스피어만 그린다.

나중에 Niagara나 사운드를 붙일 때도 이 구조를 확장할 수 있다.

---

## 4. 발사 흐름

### 1. 입력 측

`ABaseCharacter::OnFirePressed`

```cpp
CurrentWeapon->StartFire();
```

소유 캐릭터가 입력을 받고 현재 무기에게 발사를 요청한다.

---

### 2. 무기 로컬 요청

`ABaseWeapon::StartFire`

```cpp
Server_Fire(OwningCharacter->GetActorForwardVector());
```

여기서 클라이언트는 trace 결과를 만들지 않는다.

요청하는 정보는 발사 방향뿐이다.

---

### 3. 서버 검증

`ABaseWeapon::Server_Fire_Implementation`

서버에서 확인하는 것:

1. 소유 캐릭터가 있는가
2. 쿨다운이 끝났는가
3. 탄약이 남아있는가

이번에 같이 고친 버그:

```cpp
if (CurrentAmmo <= 0)
{
    UE_LOG(...);
    return;
}
```

기존에는 탄약 없음 로그만 찍고 return을 안 해서, 탄약이 0 이하로 내려갈 수 있었다.

---

### 4. 서버 히트스캔

`ABaseWeapon::PerformHitScanFire`

```cpp
const int32 SafePelletCount = FMath::Max(1, PelletCount);
const float SpreadRadians = FMath::DegreesToRadians(FMath::Max(0.0f, SpreadAngleDegrees));
```

방어 코드:

- pellet 개수는 최소 1개
- spread는 최소 0도

pellet 방향:

```cpp
const FVector PelletDirection = (SafePelletCount > 1 && SpreadRadians > 0.0f)
    ? FMath::VRandCone(AimDirection, SpreadRadians)
    : AimDirection;
```

의미:

- pellet이 1개거나 spread가 0이면 정면으로 쏜다.
- pellet이 여러 개이고 spread가 있으면 정면 방향 원뿔 안에서 랜덤 방향을 만든다.

중요:

- 이 랜덤 spread는 서버에서 만들어진다.
- 클라이언트가 pellet 방향을 정하지 않는다.

---

### 5. 서버 데미지 적용

각 pellet마다:

```cpp
HitActor->TakeDamage(
    Damage,
    DmgEvent,
    OwningCharacter ? OwningCharacter->GetController() : nullptr,
    this);
```

샷건은 pellet 하나마다 `Damage`가 적용된다.

예:

- `Damage = 8`
- `PelletCount = 8`
- 전부 맞으면 총 64 데미지

지금은 damage falloff나 같은 대상 중복 hit 제한이 없다.

이건 현재 단순 버전의 의도된 한계다.

---

## 5. 네트워크 관점 실행 흐름

```text
Owning client:
  Fire input
  → CurrentWeapon->StartFire()
  → Server_Fire(AimDirection)

Server:
  소유자/쿨다운/탄약 검증
  → 탄약 감소
  → 서버 위치 기준 TraceStart 계산
  → pellet 방향 여러 개 생성
  → 각 pellet 라인트레이스
  → 맞은 액터에 TakeDamage
  → TraceEnds/HitActors 배열로 Multicast_PlayFireFX

All clients:
  디버그 라인/스피어 표시
  → 실제 체력 변화는 각 대상 Actor의 replication으로 수신
```

핵심:

- 발사 요청은 owning client가 한다.
- 실제 hit 판정과 damage는 서버가 한다.
- 클라이언트는 FX와 replicated health 결과를 본다.

---

## 6. 왜 배열 multicast를 썼나

샷건 pellet이 8개라면 단순하게는 `Multicast_PlayFireFX`를 8번 부를 수도 있다.

하지만 그렇게 하면:

- RPC 호출 수가 늘어난다.
- “한 번의 발사”가 네트워크 이벤트 8개로 쪼개진다.
- 나중에 muzzle flash/sound 같은 한 번만 재생해야 하는 연출과 섞이기 쉽다.

그래서 한 번의 발사에 대해 multicast 한 번:

```text
TraceStart 1개
TraceEnds 여러 개
HitActors 여러 개
```

이 구조가 더 깔끔하다.

---

## 7. BP_Shotgun 셋업 방법

에디터에서:

1. `Content/Game/Blueprints/Weapon/` 이동
2. `BP_Rifle`을 복제하거나 `BaseWeapon` 부모 Blueprint 생성
3. 이름: `BP_Shotgun`
4. Details에서 값 조정

권장 시작값:

| 항목 | 값 |
|---|---|
| WeaponName | Shotgun |
| Damage | 8 |
| FireRate | 0.9 |
| MaxAmmo | 6 |
| Range | 2800 |
| PelletCount | 8 |
| SpreadAngleDegrees | 8 |

임시 테스트:

1. `BP_BaseCharacter` 열기
2. Default Weapon Class를 `BP_Shotgun`으로 변경
3. PIE Listen Server 실행
4. 발사 시 노란 디버그 라인이 여러 개 퍼지는지 확인

---

## 8. 테스트 체크리스트

- [ ] 기존 `BP_Rifle`이 여전히 한 줄만 발사된다.
- [ ] 탄약이 0이 되면 더 이상 발사되지 않는다.
- [ ] `BP_Shotgun`에서 한 번 발사하면 여러 디버그 라인이 퍼진다.
- [ ] 서버 로그에 pellet hit 로그가 찍힌다.
- [ ] remote client가 쐈을 때도 서버에서 hit/damage가 처리된다.
- [ ] 호스트와 remote client 양쪽 화면에서 같은 pellet debug FX가 보인다.
- [ ] 적에게 맞으면 `AAlienBase::TakeDamage`가 서버에서 실행되고 체력이 복제된다.

---

## 9. 현재 한계

현재 구현은 샷건의 네트워크/히트스캔 기반만 만든 상태다.

아직 없는 것:

- `BP_Shotgun` 실제 에셋
- 무기 스왑
- 재장전
- pellet damage falloff
- 같은 대상에 대한 최대 pellet hit 제한
- pellet별 물리 표면 이펙트
- 샷건 전용 사운드/VFX
- 서버 검증용 aim angle 제한

다음 확장 후보:

1. `BP_Shotgun` 생성/튜닝
2. 무기 스왑 또는 임시 숫자키 교체
3. 유탄 발사기용 projectile actor
4. pickup 시스템

---

## 10. 발표/회고 때 말할 수 있는 학습 포인트

- 기존 라이플 히트스캔을 유지하면서, 샷건을 위해 pellet/spread 개념을 추가했다.
- 클라이언트는 발사 방향만 요청하고, 실제 trace 시작점과 pellet spread는 서버에서 계산한다.
- 서버가 각 pellet hit와 damage를 판정한다.
- 클라이언트에는 한 번의 multicast로 여러 trace FX를 전달한다.
- 탄약은 기존 replicated `CurrentAmmo` 흐름을 그대로 사용한다.

짧게 정리하면:

> Step 6a는 “같은 히트스캔 무기라도 네트워크 비용과 서버 판정 위치를 의식하면서 다발 발사를 확장하는 단계”다.
