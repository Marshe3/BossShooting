# Step 8a BossQueen Phase Verification Update

작성일: 2026-05-15

## 결과

`BP_BossQueen` 생성/배치 후 Listen Server PIE에서 BossQueen 페이즈 전환이 정상 동작하는 것을 확인했다.

## 확인한 것

- `BP_BossQueen` 부모 클래스: `BossQueen`
- 맵 배치 완료
- HP 60% 이하에서 Phase2 전환 확인
- HP 30% 이하에서 Phase3 전환 확인
- 사망 시 Dead phase 전환 확인
- listen-server host와 remote client 양쪽에서 같은 결과 확인

## 네트워크 의미

BossQueen의 phase 판단은 서버가 한다. 클라이언트는 `CurrentPhase` replicated property를 받아서 같은 페이즈 결과를 관찰한다.

즉, 이 단계의 핵심은 "클라이언트가 보스 페이즈를 직접 계산하는 구조"가 아니라, 서버가 HP 비율을 보고 phase를 결정하고 그 결과를 복제하는 구조라는 점이다.

## 다음 단계

Step 8b에서는 이 phase 상태 위에 보스 Phase1 공격 패턴을 붙인다.

Phase1에서 우선 구현할 후보:

- 근접 범위 휘두르기
- 산성 projectile 3방향 발사
- 서버 타이머 기반 패턴 선택
- 공격 결과는 서버 권위, 시각 효과는 multicast 또는 Blueprint hook으로 분리

