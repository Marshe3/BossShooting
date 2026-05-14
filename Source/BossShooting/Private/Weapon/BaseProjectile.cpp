// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/BaseProjectile.h"

#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

ABaseProjectile::ABaseProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// Projectile은 서버에서 spawn되고, 위치/회전은 movement replication으로 클라에 보인다.
	bReplicates = true;
	SetReplicateMovement(true);

	// gameplay collision은 projectile actor가 담당한다. mesh/VFX는 나중에 BP에서 붙일 예정.
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(15.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionComponent->OnComponentHit.AddDynamic(this, &ABaseProjectile::OnProjectileHit);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->MaxSpeed = InitialSpeed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
}

void ABaseProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (ProjectileMovement)
	{
		// BP에서 InitialSpeed를 바꾼 경우 movement component에도 런타임 값으로 다시 반영한다.
		ProjectileMovement->InitialSpeed = InitialSpeed;
		ProjectileMovement->MaxSpeed = InitialSpeed;
	}

	if (HasAuthority())
	{
		// 서버에서만 수명 관리. 서버 actor가 사라지면 클라의 replicated actor도 정리된다.
		SetLifeSpan(LifeSeconds);
	}
}

void ABaseProjectile::InitProjectile(const FVector& LaunchDirection, AController* NewInstigatorController, AActor* NewDamageCauser, AActor* NewIgnoredActor)
{
	// 데미지 credit과 ignore 처리를 위해 발사 정보를 저장한다.
	// DamageCauser는 "누가 데미지를 냈는가", IgnoredActor는 "누구와 충돌/폭발 데미지를 무시할 것인가"에 가깝다.
	InstigatorController = NewInstigatorController;
	DamageCauser = NewDamageCauser;
	IgnoredActor = NewIgnoredActor;

	if (DamageCauser)
	{
		SetOwner(DamageCauser);
		CollisionComponent->IgnoreActorWhenMoving(DamageCauser, true);
	}

	if (IgnoredActor)
	{
		CollisionComponent->IgnoreActorWhenMoving(IgnoredActor, true);
	}

	const FVector SafeDirection = LaunchDirection.IsNearlyZero() ? GetActorForwardVector() : LaunchDirection.GetSafeNormal();
	ProjectileMovement->Velocity = SafeDirection * InitialSpeed;
}

void ABaseProjectile::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 클라에서도 충돌 이벤트가 보일 수 있지만, 폭발/데미지는 서버에서만 확정한다.
	if (!HasAuthority() || bExploded)
	{
		return;
	}

	if (OtherActor && OtherActor == DamageCauser)
	{
		return;
	}

	if (OtherActor && OtherActor == IgnoredActor)
	{
		return;
	}

	const FVector ExplosionLocation = Hit.ImpactPoint.IsNearlyZero()
		? GetActorLocation()
		: FVector(Hit.ImpactPoint);
	Explode_ServerOnly(ExplosionLocation);
}

void ABaseProjectile::Explode_ServerOnly(const FVector& ExplosionLocation)
{
	// radial damage는 gameplay 결과이므로 서버만 처리한다.
	// 클라는 아래 Multicast_Explode로 폭발 디버그 FX만 본다.
	if (!HasAuthority() || bExploded)
	{
		return;
	}

	bExploded = true;

	TArray<AActor*> IgnoredActors;
	if (DamageCauser)
	{
		IgnoredActors.Add(DamageCauser);
	}
	if (IgnoredActor)
	{
		IgnoredActors.Add(IgnoredActor);
	}

	UGameplayStatics::ApplyRadialDamage(
		this,
		Damage,
		ExplosionLocation,
		DamageRadius,
		UDamageType::StaticClass(),
		IgnoredActors,
		this,
		InstigatorController,
		true);

	UE_LOG(LogTemp, Warning, TEXT("[서버] %s 폭발: 위치=%s 데미지=%.1f 반경=%.1f"),
		*GetName(), *ExplosionLocation.ToString(), Damage, DamageRadius);

	Multicast_Explode(ExplosionLocation);
	Destroy();
}

void ABaseProjectile::Multicast_Explode_Implementation(FVector_NetQuantize ExplosionLocation)
{
	// 임시 시각화. 나중에 Niagara/SFX로 교체할 자리다.
	DrawDebugSphere(GetWorld(), ExplosionLocation, DamageRadius, 24, FColor::Orange, false, 3.0f, 0, 2.0f);
}
