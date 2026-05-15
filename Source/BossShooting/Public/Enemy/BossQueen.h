// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/AlienBase.h"
class ABaseCharacter;
class ABaseProjectile;
class AAlienBase;
#include "BossQueen.generated.h"

UENUM(BlueprintType)
enum class EBossQueenPhase : uint8
{
	Phase1,
	Phase2,
	Phase3,
	Dead
};

UCLASS()
class BOSSSHOOTING_API ABossQueen : public AAlienBase
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABossQueen();
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void HandleDeath() override;
	
	// 클라는 이 replicated phase를 보고 HUD/연출을 맞출 수 있다.
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase, VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Phase")
	EBossQueenPhase CurrentPhase = EBossQueenPhase::Phase1;
	
	// Phase2에서 일시 무적 같은 룰을 표현하기 위한 replicated 상태
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Phase")
	bool bIsInvincible = false;
	
	// HP가 MaxHealth의 60%이하가 되면 Phase2로 전환
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Phase2HealthRatio = 0.6f;
	
	// HP가 MaxHealth의 30% 이하가 되면 Phase3로 전환
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Phase3HealthRatio = 0.3f;
	
	// Phase1 보스 공격 판단 주기. 서버 타이머에서만 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase1 Attack")
	float Phase1AttackInterval = 2.0f;
	
	// 가까운 플레이어가 이 거리 안에 있으면 근접 휘두리기를 시도한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase1 Attack", meta = (ClampMin = "0.0"))
	float Phase1MeleeRange = 260.0f;
	
	// 보스 정면 기준 좌우 각도. 55면 총 110도 부채꼴이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase1 Attack", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float Phase1MeleeHalfAngleDegrees = 55.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase1 Attack", meta = (ClampMin = "0.0"))
	float Phase1MeleeDamage = 20.0f;
	
	// 멀리 있는 플레이어에게 쏠 projectile. BP_BossQueen에서 BP_AcidProjectile 같은 클래스를 넣는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase1 Attack")
	TSubclassOf<ABaseProjectile> Phase1ProjectileClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase1 Attack", meta = (ClampMin = "0.0"))
	float Phase1ProjectileSpawnForwardOffset = 140.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase1 Attack", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float Phase1ProjectileSpreadDegrees = 18.0f;
	
	// Phase2에 들어가면 서버가 이 적 클래스들 중 하나를 골라 소환한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase2 Summon")
	TArray<TSubclassOf<AAlienBase>> Phase2SummonClasses;
	
	// Phase2에서 한 번에 소환할 잡몹 수 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase2 Summon", meta = (ClampMin = "0"))
	int32 Phase2SummonCount = 8;
	
	// 보스 주변 어느 정도 거리 밖에 소환할지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase2 Summon", meta = (ClampMin = "0.0"))
	float Phase2SummonRadius = 550.0f;
	
	// 소환몹이 모두 정리됐는지 서버가 확인하는 주기
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase2 Summon", meta = (ClampMin = "0.1"))
	float Phase2MinionCheckInterval = 0.5f;
	
	// Phase3 격노 상태에서 사용할 이동 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase3 Rage", meta = (ClampMin = "0.0"))
	float Phase3WalkSpeed = 650.0f;

	// Phase3 공격 판단 주기. Phase1보다 짧게 두면 더 공격적으로 느껴진다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase3 Rage", meta = (ClampMin = "0.1"))
	float Phase3AttackInterval = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase3 Rage", meta = (ClampMin = "0.0"))
	float Phase3MeleeRange = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase3 Rage", meta = (ClampMin = "0.0"))
	float Phase3MeleeDamage = 30.0f;

	// Phase3 원거리 발사 개수. 홀수 추천: 3, 5, 7
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase3 Rage", meta = (ClampMin = "1"))
	int32 Phase3ProjectileCount = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Phase3 Rage", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float Phase3ProjectileTotalSpreadDegrees = 48.0f;

	
	UFUNCTION()
	void OnRep_CurrentPhase(EBossQueenPhase OldPhase);
	
private:
	void UpdatePhase_ServerOnly();
	void SetPhase_ServerOnly(EBossQueenPhase NewPhase);
	
	FTimerHandle BossAttackTimerHandle;
	
	FTimerHandle Phase2MinionCheckTimerHandle;
	
	UPROPERTY()
	TArray<TObjectPtr<AAlienBase>> Phase2SpawnedMinions;
	
	bool bPhase2SummonsStarted = false;
	
	float OriginalWalkSpeed = 0.0f;
	
	void StartBossAttackTimer_ServerOnly();
	void StopBossAttackTimer_ServerOnly();
	void TryPhase1Attack_ServerOnly();
	
	void EnterPhase2_ServerOnly();
	void ExitPhase2Invincibility_ServerOnly();
	void SpawnPhase2Minions_ServerOnly();
	void CheckPhase2Minions_ServerOnly();
	
	void EnterPhase3_ServerOnly();
	void StartPhase3AttackTimer_ServerOnly();
	void TryPhase3Attack_ServerOnly();
	void DoPhase3MeleeSweep_ServerOnly();
	void DoPhase3ProjectileBurst_ServerOnly(ABaseCharacter* Target);
	
	ABaseCharacter* FindClosestLivingPlayer_ServerOnly() const;
	void DoPhase1MeleeSweep_ServerOnly();
	void DoPhase1ProjectileBurst_ServerOnly(ABaseCharacter* Target);
	
	void FaceTarget_ServerOnly(ABaseCharacter* Target);
	
};
