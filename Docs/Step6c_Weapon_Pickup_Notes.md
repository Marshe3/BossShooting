# Step 6c 무기 픽업 / 1슬롯 교체 구현 노트

> 목적: 맵에 놓인 pickup actor를 서버 권위로 claim하고, 플레이어의 현재 무기를 새 무기로 교체하는 흐름을 학습한다.

이 문서는 2026-05-14 기준 Step 6c 코드 변경을 나중에 다시 공부하기 쉽게 풀어쓴 기록이다.

---

## 1. 이번에 만든 기능

`AWeaponPickup` C++ 클래스를 추가해서 맵에 무기 픽업을 배치할 수 있는 기반을 만들었다.

현재 목표는 “무기 인벤토리”가 아니라 더 단순한 1슬롯 교체 모델이다.

1. 맵에 `BP_WeaponPickup`을 놓는다.
2. pickup BP의 `WeaponClass`에 `BP_Rifle`, `BP_Shotgun`, `BP_GrenadeLauncher` 중 하나를 넣는다.
3. 플레이어가 overlap하면 서버가 pickup 가능 여부를 판단한다.
4. 서버가 `WeaponClass` 무기를 spawn한다.
5. 서버가 플레이어의 기존 무기를 제거하고 새 무기를 장착한다.
6. pickup actor는 서버에서 destroy되고, 클라이언트 화면에서도 사라진다.

---

## 2. 왜 픽업은 서버에서 처리해야 하나

픽업은 “누가 아이템을 얻었는가”를 결정하는 gameplay state다.

클라이언트가 직접 무기를 바꾸면:

- 두 플레이어가 동시에 먹었을 때 누가 얻었는지 충돌할 수 있음
- 클라이언트가 멀리 있는 pickup을 먹었다고 속일 수 있음
- 어떤 화면에서는 pickup이 사라졌고, 다른 화면에서는 남아 있는 상태가 될 수 있음

그래서 pickup claim은 서버에서만 처리한다.

| 머신 | 하는 일 |
|---|---|
| server | overlap 처리, dead 체크, WeaponClass spawn, EquipWeapon 호출, pickup destroy |
| owning client | 자기 캐릭터가 새 무기를 장착한 결과를 replication으로 봄 |
| remote clients | 다른 플레이어의 무기 교체와 pickup 제거 결과를 replication으로 봄 |

핵심 문장:

> 클라이언트는 pickup을 “먹었다고 결정”하지 않고, 서버가 결정한 결과를 본다.

---

## 3. 변경 파일 요약

### `WeaponPickup.h/cpp`

새 C++ 클래스:

```cpp
class AWeaponPickup : public AActor
```

주요 컴포넌트:

```cpp
USphereComponent* PickupCollision;
UStaticMeshComponent* PickupMesh;
```

주요 설정값:

```cpp
TSubclassOf<ABaseWeapon> WeaponClass;
```

의미:

- `PickupCollision`: 플레이어 overlap 감지용 sphere
- `PickupMesh`: 에디터에서 보이게 만들 임시/최종 mesh
- `WeaponClass`: pickup을 먹었을 때 spawn할 무기 BP class

기본 네트워크 설정:

```cpp
bReplicates = true;
```

pickup actor 자체가 서버에서 destroy되면 클라이언트에서도 사라져야 하므로 replicated actor로 만들었다.

---

### `BaseCharacter.cpp`

`ABaseCharacter::EquipWeapon`을 1슬롯 교체 모델에 맞게 수정했다.

```cpp
if (CurrentWeapon)
{
    ABaseWeapon* OldWeapon = CurrentWeapon;
    OldWeapon->DetachFromOwner();
    OldWeapon->Destroy();
}
```

의미:

- 기존 무기를 캐릭터에서 떼어낸다.
- 이전 무기 actor를 제거한다.
- 새 무기를 `CurrentWeapon`으로 설정하고 attach한다.

이 프로젝트의 현재 무기 모델은 “한 번에 무기 1개”이므로, pickup을 먹으면 기존 무기를 바닥에 떨어뜨리지 않고 제거한다.

---

## 4. Pickup 실행 흐름

### 1. BeginPlay

```cpp
if (HasAuthority())
{
    PickupCollision->OnComponentBeginOverlap.AddDynamic(this, &AWeaponPickup::OnPickupOverlap);
}
```

overlap 이벤트 바인딩을 서버에서만 한다.

이렇게 하면 클라이언트에서 overlap이 발생해도 gameplay claim 로직이 실행되지 않는다.

---

### 2. Overlap

```cpp
if (!HasAuthority() || bClaimed)
{
    return;
}
```

서버가 아니거나 이미 누가 먹은 pickup이면 무시한다.

그 다음:

```cpp
ABaseCharacter* Character = Cast<ABaseCharacter>(OtherActor);
if (!Character || Character->IsDead())
{
    return;
}
```

플레이어 캐릭터인지, 살아 있는지 확인한다.

---

### 3. Claim

```cpp
ClaimPickup_ServerOnly(Character);
```

서버에서:

1. `WeaponClass`가 설정되어 있는지 확인
2. `bClaimed = true`로 중복 claim 방지
3. 새 weapon actor spawn
4. `Character->EquipWeapon(NewWeapon)` 호출
5. pickup actor destroy

```cpp
ABaseWeapon* NewWeapon = GetWorld()->SpawnActor<ABaseWeapon>(
    WeaponClass,
    GetActorLocation(),
    GetActorRotation(),
    Params);
```

spawn된 weapon은 `EquipWeapon` 안에서 캐릭터에 attach된다.

---

## 5. 네트워크 관점 실행 흐름

```text
Server:
  Player overlaps pickup collision
  -> OnPickupOverlap
  -> alive character인지 확인
  -> WeaponClass spawn
  -> Character->EquipWeapon(NewWeapon)
  -> 기존 CurrentWeapon detach/destroy
  -> CurrentWeapon = NewWeapon
  -> weapon attach
  -> pickup Destroy()

Owning client:
  CurrentWeapon replication / OnRep 흐름으로 새 무기 상태를 봄

Remote clients:
  다른 플레이어의 새 weapon attach 결과를 봄
  pickup actor destroy replication으로 pickup이 사라진 것을 봄
```

중요:

- pickup claim은 서버 전용이다.
- `WeaponClass`는 BP에서 지정하는 서버용 설정값이다.
- pickup이 사라지는 것은 actor destroy replication 결과다.
- 무기 장착 결과는 기존 `CurrentWeapon` replication / weapon actor replication 흐름을 탄다.

---

## 6. BP 셋업 방법

### `BP_WeaponPickup`

추천 경로:

```text
Content/Game/Blueprints/Weapon/BP_WeaponPickup
```

이유:

- 이 pickup은 “무기를 주는 Actor”라서 현재 프로젝트 구조상 `Weapon` 폴더가 가장 자연스럽다.
- 나중에 체력팩, 탄약팩, 상호작용 아이템이 생기면 `Blueprints/Pickup/`으로 분리해도 된다.

에디터에서:

1. `Content/Game/Blueprints/Weapon/` 이동
2. Blueprint Class 생성
3. 부모 클래스: `WeaponPickup`
4. 이름: `BP_WeaponPickup`
5. Details에서 `WeaponClass` 지정
6. `PickupMesh`에 임시 static mesh 지정
7. 맵에 배치

테스트용으로는 pickup BP를 여러 개 복제해서 이렇게 나눠도 좋다.

| BP 이름 | WeaponClass |
|---|---|
| `BP_Pickup_Rifle` | `BP_Rifle` |
| `BP_Pickup_Shotgun` | `BP_Shotgun` |
| `BP_Pickup_GrenadeLauncher` | `BP_GrenadeLauncher` |

---

## 7. 테스트 체크리스트

- [ ] `BP_WeaponPickup` 부모가 `WeaponPickup`이다.
- [ ] `WeaponClass`가 비어 있지 않다.
- [ ] `PickupMesh`가 보여서 맵에서 위치를 확인할 수 있다.
- [ ] PIE Listen Server에서 host가 먹으면 기존 무기가 새 무기로 바뀐다.
- [ ] remote client가 먹어도 서버 로그에 pickup 로그가 찍힌다.
- [ ] remote client가 먹은 결과가 host 화면에도 보인다.
- [ ] pickup actor가 host/remote client 화면 모두에서 사라진다.
- [ ] 죽은 캐릭터는 pickup을 먹지 못한다.
- [ ] `WeaponClass`가 비어 있으면 warning 로그가 뜨고 pickup이 사라지지 않는다.

---

## 8. 현재 한계

아직 없는 것:

- 상호작용 키 기반 pickup
- pickup UI prompt
- 무기 드롭
- 인벤토리 2슬롯 이상
- ammo/reload 연동
- pickup respawn
- pickup별 rarity/outline/VFX
- 동시 overlap 경쟁 상황에 대한 더 정교한 UX

현재 구현은 “서버 권위 pickup claim + 1슬롯 weapon replacement”를 학습하기 위한 최소 버전이다.

---

## 9. 자주 헷갈리는 부분

### `bClaimed`를 왜 replicate하지 않았나

`bClaimed`는 서버 내부에서 중복 claim을 막는 guard다.

클라이언트가 `bClaimed` 값을 직접 보고 UI를 바꾸는 구조가 아니므로 지금은 replicate할 필요가 없다.

클라이언트에게 중요한 결과는 두 가지다.

- pickup actor가 사라짐
- 플레이어 무기가 바뀜

이 둘은 이미 actor destroy와 `CurrentWeapon` replication으로 전달된다.

### `WeaponClass`를 왜 replicate하지 않았나

`WeaponClass`는 pickup BP에 미리 지정하는 디자인 데이터다.

서버가 pickup을 처리할 때만 필요하다. 클라이언트 UI에서 “무슨 무기인지”를 표시해야 하는 단계가 오면, 그때 표시용 replicated data나 DataAsset 기반 표시 정보를 추가하면 된다.

---

## 10. 발표/회고 때 말할 수 있는 학습 포인트

- pickup은 단순 overlap처럼 보여도 “누가 보상을 얻는가”를 정하는 gameplay state라 서버 권위로 처리해야 한다.
- 클라이언트는 pickup claim을 직접 결정하지 않고, 서버가 spawn/equip/destroy한 결과를 replication으로 본다.
- `bClaimed`는 동시 overlap에서 같은 pickup이 두 번 처리되지 않게 하는 서버 내부 guard다.
- 현재는 1슬롯 무기 모델이라 기존 무기는 destroy하고 새 무기를 장착한다.
- 나중에 inventory나 drop 시스템을 만들면 `EquipWeapon`의 “기존 무기 제거” 부분이 확장 포인트가 된다.

짧게 정리하면:

> Step 6c는 “맵에 있는 보상을 누가 얻었는지 서버가 판정하고, 그 결과인 무기 교체와 pickup 제거만 클라이언트가 보는 구조”를 만든 단계다.
