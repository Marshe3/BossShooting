// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "AlienBase.generated.h"

UCLASS()
class BOSSSHOOTING_API AAlienBase : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAlienBase();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	
	// 사망 상태 체크 - BP에서도 쓸 수 있게
	UFUNCTION(BlueprintPure, Category = "Alien|State")
	bool IsDead() const { return CurrentHealth <= 0.0f; }
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// === Stats ===
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, BlueprintReadOnly, Category = "Alien|Stats")
	float CurrentHealth;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Alien|Stats")
	float MaxHealth = 30.0f; // 러셔 기본값
	
	
	UFUNCTION()
	void OnRep_Health(float OldHealth);
	
	// 사망처리 (서버에서만 호출, 자식 클래스에서 오버라이드 가능)
	virtual void HandleDeath();
	
	// 사망 연출 - 모든 클라에서 시각 처리
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_OnDeath();
	
	
	
protected:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditDefaultsOnly, Category = "Alien|Melee")
	bool bCanMeleeAttack = true;

	UPROPERTY(EditDefaultsOnly, Category = "Alien|Melee", meta = (EditCondition = "bCanMeleeAttack", ClampMin = "0.0"))
	float MeleeAttackRange = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Alien|Melee", meta = (EditCondition = "bCanMeleeAttack", ClampMin = "0.0"))
	float MeleeAttackDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Alien|Melee", meta = (EditCondition = "bCanMeleeAttack", ClampMin = "0.0"))
	float MeleeAttackCooldown = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Alien|Melee", meta = (EditCondition = "bCanMeleeAttack", ClampMin = "0.05"))
	float MeleeAttackCheckInterval = 0.2f;

	// 근접 공격은 Tick 대신 서버 타이머로 주기 검사한다.
	// 모든 프레임이 아니라 일정 간격으로만 range/cooldown을 확인해서 읽기 쉽고 비용도 낮다.
	FTimerHandle MeleeAttackTimerHandle;
	float LastMeleeAttackTime = -1000.0f;

	void StartMeleeAttackTimer_ServerOnly();
	void StopMeleeAttackTimer_ServerOnly();
	void TryMeleeAttack_ServerOnly();

	class ABaseCharacter* FindClosestMeleeTarget_ServerOnly() const;
};
