// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/AlienBase.h"
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
	
	UFUNCTION()
	void OnRep_CurrentPhase(EBossQueenPhase OldPhase);
	
private:
	void UpdatePhase_ServerOnly();
	void SetPhase_ServerOnly(EBossQueenPhase NewPhase);
};
