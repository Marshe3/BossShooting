# Step 8b BossQueen Phase1 Attack Notes

작성일: 2026-05-15

## 구현한 기능

BossQueen Phase1에 기본 공격 패턴을 추가했다.

- 가까운 플레이어가 있으면 전방 부채꼴 근접 휘두르기
- 멀리 있는 플레이어가 있으면 projectile 3방향 발사
- 공격 판단, 데미지 적용, projectile spawn은 모두 서버에서 처리
- 클라이언트는 health replication, projectile replication, multicast debug FX 결과를 본다

## 변경 파일

- `Source/BossShooting/Public/Enemy/BossQueen.h`
  - Phase1 공격 튜닝값 추가
  - projectile class 설정값 추가
  - 보스 공격 타이머와 Phase1 공격 함수 선언 추가

- `Source/BossShooting/Private/Enemy/BossQueen.cpp`
  - Phase1일 때만 서버 공격 타이머 시작
  - Phase2/Phase3/Dead로 전환되면 공격 타이머 정지
  - 가장 가까운 살아있는 플레이어 탐색
  - 공격 직전 서버에서 보스가 타겟을 바라보도록 보정
  - 근접 부채꼴 데미지 처리
  - 3방향 projectile 서버 spawn 처리

## 서버/클라이언트 역할

서버:

- `StartBossAttackTimer_ServerOnly`
- `TryPhase1Attack_ServerOnly`
- `FindClosestLivingPlayer_ServerOnly`
- `FaceTarget_ServerOnly`
- `DoPhase1MeleeSweep_ServerOnly`
- `DoPhase1ProjectileBurst_ServerOnly`

클라이언트:

- 보스 회전/이동은 movement replication으로 관찰
- projectile actor는 replicated actor로 관찰
- 폭발 debug FX는 기존 projectile multicast 흐름으로 관찰
- 체력 감소는 `ABaseCharacter::CurrentHealth` replication으로 관찰

## 핵심 학습 포인트

처음에는 근접 공격이 한 번만 들어가는 것처럼 보였다. 원인은 공격 타이머가 멈춘 것이 아니라, 근접 공격이 거리뿐 아니라 보스 정면 부채꼴 안에 들어와야 맞는 구조였기 때문이다.

보스가 공격 직전에 타겟을 바라보지 않으면 플레이어가 범위 안에 있어도 `ForwardDot < MinDot` 조건에 걸려 데미지가 들어가지 않을 수 있다.

그래서 `FaceTarget_ServerOnly`를 추가해 서버에서 공격 방향을 먼저 확정한 뒤 근접/원거리 패턴을 실행하도록 고쳤다.

## 검증

Listen Server PIE에서 확인:

- 가까이 붙으면 Phase1 근접 휘두르기가 반복해서 들어감
- 멀리 있으면 3방향 projectile이 발사됨
- host/remote client 양쪽에서 결과가 동일하게 보임
- projectile과 데미지 결과가 서버 권위 흐름으로 처리됨

## 현재 한계

- 아직 공격 모션/텔레그래프가 없어 눈으로 타이밍을 읽기 어렵다.
- 근접 공격은 현재 즉시 판정이다. 나중에 montage notify나 delayed hit timing으로 바꾸면 더 자연스럽다.
- Phase2/Phase3 전용 패턴은 아직 없다.

## 다음 단계

Step 8c는 Phase2 무적 + 소환 루프다.

예상 구조:

- Phase2 진입 시 `bIsInvincible = true`
- 서버 타이머로 잡몹 소환
- 소환된 잡몹이 모두 죽거나 일정 조건을 만족하면 무적 해제
- 이후 Phase3로 넘어갈 수 있게 상태 정리

