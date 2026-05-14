// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponPickup.generated.h"

class ABaseCharacter;
class ABaseWeapon;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class BOSSSHOOTING_API AWeaponPickup : public AActor
{
	GENERATED_BODY()

public:
	AWeaponPickup();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup|Components")
	TObjectPtr<USphereComponent> PickupCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup|Components")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	// 이 pickup을 먹었을 때 서버가 spawn할 무기 BP 클래스.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Weapon")
	TSubclassOf<ABaseWeapon> WeaponClass;

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	// 서버 내부 중복 claim 방지용. 클라 UI가 읽는 값이 아니므로 현재는 replicate하지 않는다.
	bool bClaimed = false;

	// pickup 보상 적용은 서버에서만 한다.
	void ClaimPickup_ServerOnly(ABaseCharacter* Character);
};
