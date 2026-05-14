// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "BaseWeapon.generated.h"

class USKeletalMeshComponent;
class ABaseCharacter;
class ABaseProjectile;

UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
	// 즉시 라인트레이스로 맞았는지 판정하는 무기. Rifle/Shotgun이 이 흐름을 사용한다.
	HitScan,

	// 서버가 projectile actor를 spawn하고, 그 actor의 이동/충돌/데미지 흐름을 사용한다.
	Projectile
};

UCLASS()
class BOSSSHOOTING_API ABaseWeapon : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABaseWeapon();
	
	// 발사 입력
	void StartFire();
	
	void StartReload();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// 캐릭터가 이 무기를 장착할 때 호출(서버에서)
	void AttachToOwner(ABaseCharacter* NewOwner);
	
	// 캐릭터가 이 무기를 버릴 때 호출 (서버에서)
	void DetachFromOwner();
	
	// 무기 식별/디버깅용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, category = "Weapon|Identity")
	FName WeaponName = TEXT("BaseWeapon");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Identity")
	EWeaponFireMode FireMode = EWeaponFireMode::HitScan;
	
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
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats", meta = (ClampMin = "0.0"))
	float ReloadDuration = 1.5f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	int32 MaxAmmo = 30;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float Range = 5000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats", meta = (ClampMin = "1"))
	int32 PelletCount = 1;

	// 샷건용 퍼짐 각도. PelletCount가 1이면 라이플처럼 정면 한 줄만 나간다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats", meta = (ClampMin = "0.0", Units = "Degrees"))
	float SpreadAngleDegrees = 0.0f;

	// Projectile 모드에서 서버가 spawn할 탄 클래스. BP_GrenadeProjectile 같은 BP를 넣는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Projectile", meta = (EditCondition = "FireMode == EWeaponFireMode::Projectile"))
	TSubclassOf<ABaseProjectile> ProjectileClass;

	// 캐릭터 중심에서 바로 spawn하면 capsule에 겹칠 수 있어서 약간 앞에서 시작시킨다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Projectile", meta = (EditCondition = "FireMode == EWeaponFireMode::Projectile", ClampMin = "0.0"))
	float ProjectileSpawnForwardOffset = 80.0f;
	
	// === 런타임 상태 ===
	
	// 현재 탄약 - 서버 권위, 클라 UI 동기화
	UPROPERTY(ReplicatedUsing = OnRep_CurrentAmmo, VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|State")
	int32 CurrentAmmo;
	
	// 장전 중 여부 - 서버 권위, 클라는 이 값으로 장전 UI/입력 제한을 맞출 수 있다.
	UPROPERTY(ReplicatedUsing = OnRep_IsReloading, VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|State")
	bool bIsReloading = false;
	
	UFUNCTION()
	void OnRep_IsReloading();
	
	UFUNCTION()
	void OnRep_CurrentAmmo(int32 OldAmmo);
	
	// 무기 소유자 캐릭터 - Replicated 포인터, 스왑 시 클라 동기화
	UPROPERTY(ReplicatedUsing = OnRep_OwningCharacter, VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|State")
	TObjectPtr<ABaseCharacter> OwningCharacter;
	
	UFUNCTION()
	void OnRep_OwningCharacter();

	// Server RPC - owning client가 "이 방향으로 쏘고 싶다"만 요청한다.
	// 실제 hit/projectile 판정은 서버가 자신의 authoritative 위치 기준으로 처리한다.
	UFUNCTION(Server, Reliable)
	void Server_Fire(const FVector& AimDirection);
	
	UFUNCTION(Server, Reliable)
	void Server_Reload();
	
	void CompleteReload_ServerOnly();
	
	// Multicast RPC - 서버가 판정한 발사 결과를 모든 화면에 디버그 FX로 보여준다.
	// 데미지/탄약 같은 지속 상태 동기화는 Replication이 담당한다.
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFireFX(const FVector& TraceStart, const TArray<FVector>& TraceEnds, const TArray<AActor*>& HitActors);
	
	// 서버에서 실제 히트스캔 + 데미지 처리. 클라이언트는 hit 결과를 결정하지 않는다.
	void PerformHitScanFire(const FVector& TraceStart, const FVector& AimDirection);

	// 서버에서 projectile actor를 spawn하는 흐름. ProjectileClass가 비어 있으면 발사 실패.
	void SpawnProjectile_ServerOnly(const FVector& AimDirection);
	
	// 마지막 발사 시각 (쿨다운용, 서버 권위)
	float LastFireTime = 0.0f;
	
	FTimerHandle ReloadTimerHandle;
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
