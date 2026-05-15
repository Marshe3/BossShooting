# Troubleshooting: Projectile Explosion Not Visible On Remote Client

작성일: 2026-05-15

## 증상

Listen Server PIE에서 projectile 자체는 host와 remote client 양쪽에 보였다.

하지만 projectile이 충돌했을 때:

- 서버/host 쪽에서는 폭발 debug sphere가 보임
- remote client 쪽에서는 projectile은 사라지지만 폭발 debug sphere가 보이지 않음
- 데미지는 정상 적용되는 것처럼 보임

즉, gameplay 결과는 서버에서 처리되고 있었지만 remote client의 폭발 시각 피드백이 빠지는 상태였다.

## 관련 파일

- `Source/BossShooting/Public/Weapon/BaseProjectile.h`
- `Source/BossShooting/Private/Weapon/BaseProjectile.cpp`

## 기존 구조

`ABaseProjectile`은 서버에서 spawn되고 movement replication으로 클라이언트에 보인다.

충돌 시 서버에서만 `Explode_ServerOnly()`가 실행된다.

기존 폭발 흐름:

```cpp
UGameplayStatics::ApplyRadialDamage(...);
Multicast_Explode(ExplosionLocation);
Destroy();
```

`Multicast_Explode()`는 모든 클라이언트에 폭발 debug sphere를 보여주기 위한 NetMulticast RPC였다.

## 원인 추정

projectile actor가 `Multicast_Explode()`를 호출한 직후 바로 `Destroy()`되고 있었다.

remote client 입장에서는 다음 일이 거의 동시에 도착할 수 있다.

- projectile actor destroy replication
- unreliable multicast explosion RPC

기존 multicast는 `Unreliable`이었다.

그래서 remote client가 projectile actor 제거를 먼저 처리하거나, multicast RPC를 놓치면 폭발 debug sphere를 볼 수 없다.

핵심은:

```text
projectile actor는 보이지만,
그 actor가 죽기 직전에 보낸 unreliable multicast는 remote client에서 놓칠 수 있다.
```

## 수정 내용

### 1. 폭발 multicast를 Reliable로 변경

`BaseProjectile.h`

기존:

```cpp
UFUNCTION(NetMulticast, Unreliable)
void Multicast_Explode(FVector_NetQuantize ExplosionLocation);
```

변경:

```cpp
UFUNCTION(NetMulticast, Reliable)
void Multicast_Explode(FVector_NetQuantize ExplosionLocation);
```

폭발은 매 프레임 반복되는 이벤트가 아니라 projectile당 한 번 발생하는 중요한 시각 피드백이므로 Reliable 사용이 적절하다고 판단했다.

### 2. 폭발 직후 즉시 Destroy하지 않음

`BaseProjectile.cpp`

기존:

```cpp
Multicast_Explode(ExplosionLocation);
Destroy();
```

변경:

```cpp
if (CollisionComponent)
{
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

if (ProjectileMovement)
{
	ProjectileMovement->StopMovementImmediately();
}

Multicast_Explode(ExplosionLocation);

SetLifeSpan(0.2f);
```

변경 후 흐름:

1. 서버가 radial damage 적용
2. projectile collision 비활성화
3. projectile movement 정지
4. Reliable multicast로 폭발 연출 전파
5. actor를 즉시 파괴하지 않고 0.2초 뒤 제거

## 왜 이렇게 고쳤나

폭발한 projectile을 계속 충돌 가능한 상태로 둘 수는 없다.

그래서 `Destroy()`를 늦추는 대신:

- collision을 꺼서 중복 충돌 방지
- movement를 멈춰서 projectile이 더 진행하지 않게 함
- 아주 짧은 life span만 남겨 multicast가 remote client에 도착할 시간을 확보

이 방식은 gameplay 결과와 cosmetic event를 분리한다.

```text
서버: 데미지 판정 즉시 완료
클라: 폭발 연출을 Reliable multicast로 수신
actor 제거: 짧은 지연 후 처리
```

## 검증 결과

Listen Server PIE에서 확인:

- host가 쏜 런처 projectile 폭발이 remote client에서도 보임
- 적/보스 원거리 projectile 폭발이 remote client에서도 보임
- 데미지는 기존처럼 정상 적용됨

## 학습 포인트

### 1. Actor replication과 Multicast RPC는 별개다

projectile actor가 client에 보인다고 해서, 그 actor가 보낸 multicast event도 항상 보장되는 것은 아니다.

특히 actor가 곧바로 destroy되면 remote client에서 one-shot cosmetic RPC를 놓칠 수 있다.

### 2. 중요한 one-shot feedback은 Reliable을 고려할 수 있다

총알 이동이나 aim update처럼 자주 반복되는 이벤트는 Reliable로 보내면 안 된다.

하지만 폭발, 사망, phase 전환 같은 낮은 빈도의 중요한 피드백은 Reliable multicast가 더 적절할 수 있다.

### 3. Destroy 직전 multicast는 조심해야 한다

다음 패턴은 remote client에서 불안정할 수 있다.

```cpp
Multicast_PlayFX();
Destroy();
```

더 안전한 패턴:

```cpp
DisableCollision();
StopMovement();
Multicast_PlayFX();
SetLifeSpan(ShortDelay);
```

## 다음 개선 후보

현재 폭발 연출은 `DrawDebugSphere`다.

나중에 더 게임답게 만들려면:

- `Multicast_Explode()`에서 Niagara explosion spawn
- projectile mesh hide 처리
- trail Niagara 제거
- hit sound multicast
- 폭발 decal 또는 acid puddle actor 추가

하지만 gameplay/network 학습 관점에서는 현재 구조가 충분히 명확하다.

