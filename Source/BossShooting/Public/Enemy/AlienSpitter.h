// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/AlienBase.h"
#include "AlienSpitter.generated.h"

class ABaseCharacter;
class ABaseProjectile;

UCLASS()
class BOSSSHOOTING_API AAlienSpitter : public AAlienBase
{
	GENERATED_BODY()

public:
	AAlienSpitter();

protected:
	virtual void BeginPlay() override;
	virtual void HandleDeath() override;

	UPROPERTY(EditDefaultsOnly, Category = "Alien|Ranged")
	bool bCanRangedAttack = true;

	// 스피터가 서버에서 spawn할 산성 침 projectile BP.
	UPROPERTY(EditDefaultsOnly, Category = "Alien|Ranged", meta = (EditCondition = "bCanRangedAttack"))
	TSubclassOf<ABaseProjectile> AcidProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Alien|Ranged", meta = (EditCondition = "bCanRangedAttack", ClampMin = "0.0"))
	float RangedAttackRange = 1200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Alien|Ranged", meta = (EditCondition = "bCanRangedAttack", ClampMin = "0.0"))
	float RangedAttackCooldown = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Alien|Ranged", meta = (EditCondition = "bCanRangedAttack", ClampMin = "0.05"))
	float RangedAttackCheckInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Alien|Ranged", meta = (EditCondition = "bCanRangedAttack", ClampMin = "0.0"))
	float ProjectileSpawnForwardOffset = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Alien|Ranged", meta = (EditCondition = "bCanRangedAttack", ClampMin = "0.0"))
	float ProjectileSpawnUpOffset = 60.0f;

private:
	// 원거리 공격도 Tick 대신 서버 타이머로 검사한다.
	FTimerHandle RangedAttackTimerHandle;
	float LastRangedAttackTime = -1000.0f;

	void StartRangedAttackTimer_ServerOnly();
	void StopRangedAttackTimer_ServerOnly();
	void TryRangedAttack_ServerOnly();
	void SpawnAcidProjectile_ServerOnly(ABaseCharacter* Target);

	ABaseCharacter* FindClosestRangedTarget_ServerOnly() const;
};
