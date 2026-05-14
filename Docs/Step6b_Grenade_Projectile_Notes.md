# Step 6b 유탄 Projectile 구현 노트

> 목적: hitscan과 다른 네트워크 패턴인 **서버 spawn projectile + replicated movement + radial damage**를 학습한다.

이 문서는 2026-05-14 기준 Step 6b 코드 변경을 나중에 다시 공부하기 쉽게 풀어쓴 기록이다.

---

## 1. 이번에 만든 기능

유탄 발사기용 C++ 기반을 만들었다.

아직 BP 에셋은 따로 세팅해야 하지만, C++로는 다음 흐름이 가능해졌다.

1. 무기 BP에서 `FireMode = Projectile`로 설정한다.
2. `ProjectileClass`에 `BP_GrenadeProjectile` 같은 projectile BP를 넣는다.
3. 플레이어가 발사하면 owning client가 `Server_Fire(AimDirection)`을 요청한다.
4. 서버가 탄약/쿨다운을 검증한다.
5. 서버가 projectile actor를 spawn한다.
6. projectile movement가 서버에서 움직이고 위치가 클라이언트에 복제된다.
7. projectile이 충돌하면 서버에서 폭발한다.
8. 서버가 `ApplyRadialDamage`로 반경 피해를 적용한다.
9. 모든 클라이언트에 폭발 디버그 구가 multicast로 보인다.

---

## 2. 왜 projectile은 서버에서 spawn해야 하나

유탄은 hitscan과 다르게 “날아가는 Actor”다.

클라이언트가 각자 projectile을 만들면:

- 어떤 클라는 맞았다고 보고, 다른 클라는 안 맞았다고 볼 수 있음
- 충돌 타이밍이 서로 달라질 수 있음
- 클라이언트가 폭발 위치나 데미지를 조작할 수 있음

그래서 gameplay projectile은 서버에서 spawn한다.

| 머신 | 하는 일 |
|---|---|
| owning client | 발사 입력, 발사 방향 요청 |
| server | projectile spawn, 이동 authority, 충돌 판정, radial damage |
| remote clients | 복제된 projectile 위치와 multicast 폭발 연출을 봄 |

핵심 문장:

> Projectile이 데미지를 준다면 spawn, collision, damage는 서버가 결정한다.

---

## 3. 변경 파일 요약

### `BaseProjectile.h/cpp`

새 C++ 클래스:

```cpp
class ABaseProjectile : public AActor
```

주요 컴포넌트:

```cpp
USphereComponent* CollisionComponent;
UProjectileMovementComponent* ProjectileMovement;
```

주요 튜닝값:

| 값 | 기본값 | 의미 |
|---|---:|---|
| `Damage` | 60 | 폭발 중심 데미지 |
| `DamageRadius` | 250 | 폭발 반경 |
| `InitialSpeed` | 1600 | 발사 속도 |
| `LifeSeconds` | 5 | 자동 제거 시간 |

기본 네트워크 설정:

```cpp
bReplicates = true;
SetReplicateMovement(true);
```

의미:

- 서버에 있는 projectile actor가 클라이언트에도 복제된다.
- 서버에서 움직인 위치/회전을 클라이언트가 받아서 본다.

---

## 4. Projectile 실행 흐름

### 1. 생성 직후 초기화

```cpp
void ABaseProjectile::InitProjectile(
    const FVector& LaunchDirection,
    AController* NewInstigatorController,
    AActor* NewDamageCauser)
```

여기서 하는 일:

- 누가 발사했는지 저장
- 누가 데미지 원인인지 저장
- 발사자와 충돌하지 않도록 ignore 처리
- projectile velocity 설정

```cpp
ProjectileMovement->Velocity = SafeDirection * InitialSpeed;
```

---

### 2. 충돌

```cpp
void ABaseProjectile::OnProjectileHit(...)
```

중요:

```cpp
if (!HasAuthority() || bExploded)
{
    return;
}
```

클라이언트에서도 충돌 이벤트 비슷한 것이 보일 수 있지만, 폭발과 데미지는 서버에서만 처리한다.

---

### 3. 폭발

```cpp
void ABaseProjectile::Explode_ServerOnly(const FVector& ExplosionLocation)
```

서버에서:

```cpp
UGameplayStatics::ApplyRadialDamage(
    this,
    Damage,
    ExplosionLocation,
    DamageRadius,
    UDamageType::StaticClass(),
    IgnoredActors,
    this,
    InstigatorController,
    true);
```

의미:

- 폭발 위치 주변 Actor들에게 데미지를 준다.
- `TakeDamage`를 구현한 Actor들은 기존 damage 흐름을 탄다.
- 발사자는 `IgnoredActors`에 넣어 자기 폭발 데미지는 일단 제외했다.

그 다음:

```cpp
Multicast_Explode(ExplosionLocation);
Destroy();
```

- 모든 클라이언트에 폭발 디버그 구를 보여준다.
- projectile actor를 제거한다.

---

## 5. `BaseWeapon` 변경

### FireMode 추가

```cpp
UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
    HitScan,
    Projectile
};
```

무기 BP에서 발사 방식을 고를 수 있게 했다.

| FireMode | 용도 |
|---|---|
| `HitScan` | 라이플, 샷건 |
| `Projectile` | 유탄 발사기 |

기본값은 `HitScan`이다.

그래서 기존 라이플/샷건은 그대로 유지된다.

---

### Projectile 설정값 추가

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Projectile")
TSubclassOf<ABaseProjectile> ProjectileClass;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Projectile")
float ProjectileSpawnForwardOffset = 80.0f;
```

의미:

- `ProjectileClass`: 어떤 projectile BP를 spawn할지
- `ProjectileSpawnForwardOffset`: 캐릭터 중심에서 얼마나 앞에 spawn할지

offset이 필요한 이유:

- 캐릭터 중심에서 바로 spawn하면 자기 capsule과 겹치거나 너무 안쪽에서 시작할 수 있다.
- 약간 앞에서 시작해야 발사체처럼 보인다.

---

### `Server_Fire` 분기

```cpp
if (FireMode == EWeaponFireMode::Projectile)
{
    SpawnProjectile_ServerOnly(SafeAimDirection);
}
else
{
    const FVector TraceStart = OwningCharacter->GetActorLocation();
    PerformHitScanFire(TraceStart, SafeAimDirection);
}
```

같은 fire input/RPC 흐름을 쓰지만, 서버에서 무기 모드에 따라 결과가 달라진다.

- hitscan: 즉시 trace/damage
- projectile: actor spawn 후 이동/충돌/damage

---

## 6. 네트워크 관점 실행 흐름

```text
Owning client:
  Fire input
  → CurrentWeapon->StartFire()
  → Server_Fire(AimDirection)

Server:
  쿨다운/탄약 검증
  → FireMode 확인
  → ProjectileClass spawn
  → InitProjectile
  → projectile movement authority
  → collision hit
  → ApplyRadialDamage
  → Multicast_Explode
  → Destroy projectile

All clients:
  replicated projectile movement 관찰
  → multicast explosion debug sphere 표시
  → damage 결과는 health replication으로 관찰
```

---

## 7. BP 셋업 방법

### `BP_GrenadeProjectile`

1. `Content/Game/Blueprints/Weapon/` 또는 `Content/Game/Blueprints/Projectile/`에 Blueprint 생성
2. 부모 클래스: `BaseProjectile`
3. 권장 값:

| 항목 | 값 |
|---|---:|
| Damage | 60 |
| DamageRadius | 250 |
| InitialSpeed | 1600 |
| LifeSeconds | 5 |

### `BP_GrenadeLauncher`

1. `BP_Rifle` 복제 또는 `BaseWeapon` 부모 Blueprint 생성
2. 이름: `BP_GrenadeLauncher`
3. 권장 값:

| 항목 | 값 |
|---|---:|
| WeaponName | GrenadeLauncher |
| FireMode | Projectile |
| ProjectileClass | BP_GrenadeProjectile |
| FireRate | 0.9 |
| MaxAmmo | 6 |
| ProjectileSpawnForwardOffset | 80 |

임시 테스트:

1. `BP_BaseCharacter`의 Default Weapon Class를 `BP_GrenadeLauncher`로 변경
2. PIE Listen Server 실행
3. 발사체가 날아가고 충돌 시 주황색 디버그 구가 보이는지 확인

---

## 8. 테스트 체크리스트

- [ ] `BP_GrenadeLauncher`가 `FireMode=Projectile`로 되어 있다.
- [ ] `ProjectileClass`에 `BP_GrenadeProjectile`이 들어 있다.
- [ ] 발사하면 서버 로그에 projectile 발사 로그가 찍힌다.
- [ ] projectile이 두 화면에서 움직여 보인다.
- [ ] 벽/적/플레이어에 닿으면 폭발한다.
- [ ] 폭발 디버그 구가 호스트와 remote client 모두에게 보인다.
- [ ] 폭발 반경 안의 Actor가 서버에서 damage를 받는다.
- [ ] 발사자 자신은 현재 ignored 처리되어 자기 폭발 데미지를 받지 않는다.

---

## 9. 현재 한계

아직 없는 것:

- projectile mesh/VFX
- arc/gravity 튜닝
- explosion Niagara/SFX
- 팀킬/자기 피해 정책
- radial damage falloff 세부 튜닝
- projectile owner별 collision channel 정리
- projectile prediction
- reload
- 무기 스왑

현재는 네트워크 학습용 최소 projectile이다.

그래도 핵심은 들어갔다.

> 서버가 projectile actor를 만들고, 서버가 충돌과 폭발 데미지를 결정한다.

---

## 10. 발표/회고 때 말할 수 있는 학습 포인트

- 라이플/샷건은 hitscan이고, 유탄은 projectile actor라 네트워크 패턴이 다르다.
- projectile은 서버에서 spawn하고 `bReplicateMovement`로 클라이언트에 위치를 보여준다.
- 충돌과 radial damage는 서버에서만 처리한다.
- 폭발 연출은 `NetMulticast`로 전체 클라이언트에 전달한다.
- damage 결과는 각 Actor의 health replication으로 동기화된다.

짧게 정리하면:

> Step 6b는 “즉시 판정 hitscan”에서 “서버가 spawn한 이동 Actor가 나중에 충돌해 데미지를 만드는 projectile 패턴”으로 넘어간 단계다.
