# Step 8d BossQueen Phase3 Rage Implementation Update

작성일: 2026-05-15

## 현재 상태

BossQueen Phase3 격노 패턴의 C++ 기반을 추가했고 `BossShootingEditor Win64 Development` 빌드에 성공했다.

## 구현한 것

- Phase3 진입 시 보스 무적 해제
- Phase3 진입 시 Phase2 소환몹 체크 타이머 정리
- Phase3 진입 시 `CharacterMovementComponent.MaxWalkSpeed`를 `Phase3WalkSpeed`로 변경
- Phase3 전용 공격 타이머 시작
- Phase3 근접 공격:
  - `Phase3MeleeRange`
  - `Phase3MeleeDamage`
  - 기존 부채꼴 판정 재사용
- Phase3 projectile 공격:
  - `Phase3ProjectileCount`
  - `Phase3ProjectileTotalSpreadDegrees`
  - 균등 분산 탄막 발사

## 서버/클라이언트 구조

서버:

- Phase3 진입 판단
- 이동 속도 변경
- 공격 타이머 등록
- 근접 데미지 판정
- projectile spawn

클라이언트:

- 보스 이동/회전 복제 결과 관찰
- projectile replicated actor 관찰
- 체력 변화 복제 결과 관찰

## BP 검증 필요

`BP_BossQueen`에서 Phase3 Rage 값을 튜닝하고 Listen Server PIE에서 확인한다.

추천 테스트:

- `Phase3WalkSpeed`: 650
- `Phase3AttackInterval`: 1.0
- `Phase3MeleeRange`: 300
- `Phase3MeleeDamage`: 30
- `Phase3ProjectileCount`: 5
- `Phase3ProjectileTotalSpreadDegrees`: 48

검증 순서:

1. 보스 HP를 60% 이하로 낮춰 Phase2 진입
2. Phase2 소환몹 처치 후 무적 해제 확인
3. 보스 HP를 30% 이하로 낮춰 Phase3 진입
4. 이동 속도 증가 확인
5. 공격 주기 단축 확인
6. 강화된 근접/탄막 공격이 host/remote client 양쪽에서 보이는지 확인

