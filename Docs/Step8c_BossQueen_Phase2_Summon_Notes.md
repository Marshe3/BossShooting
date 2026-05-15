# Step 8c BossQueen Phase2 Summon Notes

작성일: 2026-05-15

## 구현한 기능

BossQueen Phase2에 무적 상태와 잡몹 소환 루프를 추가했다.

- HP 60% 이하로 Phase2 진입
- Phase2 진입 시 보스 무적 활성화
- 서버가 지정된 적 BP들을 보스 주변에 소환
- 소환몹이 살아있는 동안 보스는 데미지를 무시
- 소환몹이 모두 죽거나 사라지면 보스 무적 해제

## 변경 파일

- `Source/BossShooting/Public/Enemy/BossQueen.h`
  - `Phase2SummonClasses`
  - `Phase2SummonCount`
  - `Phase2SummonRadius`
  - `Phase2MinionCheckInterval`
  - `Phase2SpawnedMinions`
  - Phase2 진입/소환/체크 함수 선언

- `Source/BossShooting/Private/Enemy/BossQueen.cpp`
  - Phase2 진입 시 `EnterPhase2_ServerOnly()` 호출
  - `bIsInvincible = true`로 데미지 무시 상태 전환
  - 서버에서 소환몹 spawn
  - 서버 타이머로 소환몹 생존 여부 확인
  - 소환몹 정리 후 `bIsInvincible = false`

## 서버/클라이언트 역할

서버:

- Phase2 진입 판단
- 보스 무적 상태 변경
- 소환몹 spawn
- 소환몹 생존 여부 체크
- 무적 해제 판단

클라이언트:

- 보스 phase와 무적 상태의 복제 결과를 관찰
- 서버가 spawn한 소환몹을 replicated actor로 관찰
- 보스/소환몹의 이동과 체력 변화를 복제 결과로 확인

## 핵심 학습 포인트

Phase2의 핵심은 “보스가 무적이 된다”가 아니라, **무적을 켜고 끄는 조건을 서버가 끝까지 책임진다**는 점이다.

클라이언트가 “소환몹 다 죽었으니 보스 때려도 됨”을 판단하면 치트나 동기화 문제가 생긴다. 그래서 서버가 `Phase2SpawnedMinions` 배열을 들고 있고, 타이머로 배열을 확인하면서 살아있는 소환몹이 없을 때만 무적을 해제한다.

## 검증

Listen Server PIE에서 확인:

- HP 60% 이하에서 Phase2 진입
- Phase2 동안 보스가 데미지를 무시
- 소환몹이 host/remote client 양쪽에 표시
- 소환몹을 모두 처치하면 보스 무적 해제
- 무적 해제 후 보스가 다시 데미지를 받음

## 현재 한계

- Phase2 소환 위치는 보스 주변 원형 배치이며 NavMesh 보정은 아직 없다.
- 무적 상태를 보여주는 별도 VFX/HUD가 없다.
- 소환몹이 많은 경우 난이도 튜닝이 필요하다.
- Phase2 종료 후 Phase3 전용 공격 패턴은 아직 없다.

## 다음 단계

Step 8d는 Phase3 격노 패턴이다.

예상 방향:

- Phase3 진입 시 이동 속도 증가
- 공격 주기 단축
- 근접 또는 projectile 패턴 강화
- Phase3 상태 변화는 서버가 결정하고 클라이언트는 복제 결과를 관찰

