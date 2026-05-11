// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseWeapon.generated.h"

class USKeletalMeshComponent;
class ABaseCharacter;


UCLASS()
class BOSSSHOOTING_API ABaseWeapon : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABaseWeapon();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// 캐릭터가 이 무기를 장착할 때 호출(서버에서)
	void AttachToOwner(ABaseCharacter* NewOwner);
	
	// 캐릭터가 이 무기를 버릴 때 호출 (서버에서)
	void DetachFromOwner();
	
	// 무기 식별/디버깅용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, category = "Weapon|Identity")
	FName WeaponName = TEXT("BaseWeapon");
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// 무기 메쉬 (스켈레탈 메쉬 - 발사 애니메이션 노티파이 노티파이 같은 거 쓸 수 있게)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;
	
	// 캐릭터에 attach될 때 사용할 소켓 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Attach")
	FName AttachSocketName = TEXT("hand_r_socket");
	
	// === Stats - 무기별 차이를 BP에서 정의 ===
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float Damage = 20.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float FireRate = 0.15f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	int32 MaxAmmo = 30;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")

	float Range = 5000.0f;
	
	// === 런타임 상태 ===
	
	// 현재 탄약 - 서버 권위, 클라 UI 동기화
	UPROPERTY(ReplicatedUsing = OnRep_OwningCharacter, VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|State")
	int32 CurrentAmmo;
	
	UFUNCTION()
	void OnRep_CurrentAmmo(int32 OldAmmo);
	
	// 무기 소유자 캐릭터 - Replicated 포인터, 스왑 시 클라 동기화
	UPROPERTY(ReplicatedUsing = OnRep_OwningCharacter, VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|State")
	TObjectPtr<ABaseCharacter> OwningCharacter;
	
	UFUNCTION()
	void OnRep_OwningCharacter();

	
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};