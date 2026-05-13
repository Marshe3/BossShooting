// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
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
	
	
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
