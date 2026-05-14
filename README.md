# Codex Unreal Top-Down Multiplayer Skills

이 폴더는 Unreal Engine C++ 기반 4인 리슨 서버 탑뷰 게임 프로젝트용 Codex 설정/스킬 세트입니다.

## 넣을 위치

프로젝트 루트에 그대로 복사하세요.

```text
YourUnrealProject/
  AGENTS.md
  .agents/
    skills/
      ue-topdown-cpp-gameplay/
        SKILL.md
      ue-listen-server-multiplayer/
        SKILL.md
      ue-build-error-debugger/
        SKILL.md
      ue-gameplay-code-reviewer/
        SKILL.md
      ue-data-driven-content/
        SKILL.md
```

## 역할

- `AGENTS.md`: 프로젝트 전체 규칙.
- `ue-topdown-cpp-gameplay`: 탑뷰 C++ 게임플레이 구현.
- `ue-listen-server-multiplayer`: 4인 리슨 서버 멀티플레이 권한, 복제, RPC.
- `ue-build-error-debugger`: 언리얼 C++ 빌드/UHT/링커/모듈 오류 해결.
- `ue-gameplay-code-reviewer`: 멀티플레이 관점 코드 리뷰.
- `ue-data-driven-content`: 몬스터, 무기, 스킬, 웨이브 등 데이터 기반 설계.

## 사용 팁

Codex에게 작업을 맡길 때 다음처럼 말하면 됩니다.

```text
Use the ue-listen-server-multiplayer skill and implement server-authoritative projectile firing.
```

또는:

```text
Use the ue-gameplay-code-reviewer skill and review this combat code for listen-server multiplayer issues.
```
