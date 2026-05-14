// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class BOSSSHOOTING_API ABaseProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABaseProjectile();

	// 서버가 spawn한 직후 호출해서 발사 방향과 데미지 책임자/무시할 actor를 세팅한다.
	void InitProjectile(const FVector& LaunchDirection, AController* NewInstigatorController, AActor* NewDamageCauser, AActor* NewIgnoredActor = nullptr);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Stats", meta = (ClampMin = "0.0"))
	float Damage = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Stats", meta = (ClampMin = "0.0"))
	float DamageRadius = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Stats", meta = (ClampMin = "0.0"))
	float InitialSpeed = 1600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Stats", meta = (ClampMin = "0.0"))
	float LifeSeconds = 5.0f;

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// 폭발 위치/반경 디버그 FX만 전체 클라에 뿌린다. 실제 radial damage는 서버에서 이미 처리한다.
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_Explode(FVector_NetQuantize ExplosionLocation);

private:
	UPROPERTY()
	TObjectPtr<AActor> DamageCauser;

	UPROPERTY()
	TObjectPtr<AActor> IgnoredActor;

	UPROPERTY()
	TObjectPtr<AController> InstigatorController;

	bool bExploded = false;

	// 충돌 후 서버에서만 호출되는 폭발/데미지 처리.
	void Explode_ServerOnly(const FVector& ExplosionLocation);
};
