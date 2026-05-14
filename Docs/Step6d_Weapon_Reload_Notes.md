# Step 6d 무기 장전 구현 노트

> 목적: 클라이언트 입력으로 장전을 요청하되, **서버가 장전 가능 여부와 탄약 복구를 결정**하는 흐름을 학습한다.

이 문서는 2026-05-14 기준 Step 6d 코드 변경을 나중에 다시 공부하기 쉽게 풀어쓴 기록이다.

---

## 1. 이번에 만든 기능

`ABaseWeapon`에 장전 기능을 추가했다.

흐름:

1. 플레이어가 `ReloadAction`을 누른다.
2. `ABaseCharacter::OnReloadPressed()`가 현재 무기의 `StartReload()`를 호출한다.
3. 무기는 `Server_Reload()` RPC로 서버에 장전을 요청한다.
4. 서버가 장전 가능 여부를 검사한다.
5. 서버가 `bIsReloading = true`로 바꾸고 timer를 시작한다.
6. timer가 끝나면 서버가 `CurrentAmmo = MaxAmmo`로 채운다.
7. `CurrentAmmo`, `bIsReloading` replication으로 클라이언트가 결과를 본다.

---

## 2. 왜 장전은 서버에서 결정해야 하나

장전은 탄약 상태를 바꾸는 gameplay action이다.

클라이언트가 직접 탄약을 채우면:

- 탄약을 무한히 채울 수 있음
- 발사/장전 타이밍이 화면마다 달라질 수 있음
- remote client가 쏜 탄약 상태와 서버 상태가 어긋날 수 있음

그래서 클라이언트는 “장전하고 싶다”만 요청하고, 서버가 실제 탄약 상태를 바꾼다.

| 머신 | 하는 일 |
|---|---|
| owning client | R키 입력, `StartReload()` 호출 |
| server | 장전 가능 여부 검사, timer 시작, 탄약 복구 |
| clients | `CurrentAmmo`, `bIsReloading` 복제 결과 관찰 |

핵심 문장:

> 장전은 입력은 클라에서 시작하지만, 탄약 복구는 서버가 확정한다.

---

## 3. 변경 파일 요약

### `BaseWeapon.h/cpp`

추가된 튜닝값:

```cpp
float ReloadDuration = 1.5f;
```

추가된 replicated state:

```cpp
UPROPERTY(ReplicatedUsing = OnRep_IsReloading)
bool bIsReloading = false;
```

추가된 함수:

```cpp
void StartReload();

UFUNCTION(Server, Reliable)
void Server_Reload();

void CompleteReload_ServerOnly();
```

`Server_Fire`에는 장전 중 발사 거부가 추가됐다.

---

### `BaseCharacter.h/cpp`

추가된 입력 슬롯:

```cpp
TObjectPtr<UInputAction> ReloadAction;
```

추가된 입력 핸들러:

```cpp
void OnReloadPressed();
```

캐릭터는 R키 입력을 현재 무기에 전달만 한다.

실제 장전 가능 여부는 무기가 서버에서 판단한다.

---

## 4. 서버 장전 검증

`Server_Reload_Implementation`에서 검사하는 것:

1. 무기 소유 캐릭터가 있는가
2. 이미 장전 중인가
3. 탄약이 이미 가득 찼는가

성공하면:

```cpp
bIsReloading = true;
OnRep_IsReloading();
```

서버는 RepNotify가 자동 호출되지 않으므로 수동 호출한다.

그 뒤 timer:

```cpp
GetWorldTimerManager().SetTimer(
    ReloadTimerHandle,
    this,
    &ABaseWeapon::CompleteReload_ServerOnly,
    SafeReloadDuration,
    false);
```

timer가 끝나면:

```cpp
CurrentAmmo = MaxAmmo;
bIsReloading = false;
```

---

## 5. BP 셋업 방법

### `IA_Reload`

추천 경로:

```text
Content/Game/Input/IA_Reload
```

설정:

| 항목 | 값 |
|---|---|
| Value Type | Digital / Bool |

### `IMC_Default`

`IA_Reload`를 추가하고 키를 `R`로 매핑한다.

### `BP_BaseCharacter`

Class Defaults에서:

| 항목 | 값 |
|---|---|
| ReloadAction | `IA_Reload` |

---

## 6. 테스트 체크리스트

- [ ] `IA_Reload`가 Digital/Bool이다.
- [ ] `IMC_Default`에 `IA_Reload`가 R키로 들어 있다.
- [ ] `BP_BaseCharacter.ReloadAction`에 `IA_Reload`가 들어 있다.
- [ ] 탄약을 몇 발 쏜 뒤 R키를 누르면 장전 시작 로그가 찍힌다.
- [ ] 장전 중 발사하면 서버에서 발사 거부 로그가 찍힌다.
- [ ] 장전 시간이 끝나면 `CurrentAmmo`가 `MaxAmmo`로 돌아온다.
- [ ] remote client가 장전해도 서버 로그와 클라 로그가 같은 흐름으로 찍힌다.

---

## 7. 현재 한계

아직 없는 것:

- 예비 탄약 reserve ammo
- 장전 취소
- 장전 애니메이션
- 무기별 장전 사운드
- HUD 장전 바
- 샷건 한 발씩 장전

현재는 “서버 권위 장전 + 탄약 복제”를 배우는 최소 버전이다.

---

## 8. 발표/회고 때 말할 수 있는 학습 포인트

- 장전은 탄약 상태를 바꾸므로 서버가 확정해야 한다.
- 클라이언트는 `Server_Reload`를 요청만 한다.
- `bIsReloading`은 클라 UI/입력 제한을 맞추기 위한 replicated state다.
- timer는 서버에서 돌고, 완료 시 서버가 `CurrentAmmo`를 채운다.
- `CurrentAmmo`와 `bIsReloading`은 RepNotify로 클라 반응 hook을 만들 수 있다.

짧게 정리하면:

> Step 6d는 “클라 입력으로 시작하지만 서버 timer와 replicated ammo state로 끝나는 장전 패턴”을 만든 단계다.
