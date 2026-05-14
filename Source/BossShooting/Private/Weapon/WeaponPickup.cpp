// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/WeaponPickup.h"

#include "Character/BaseCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Weapon/BaseWeapon.h"

AWeaponPickup::AWeaponPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	// pickup actor가 서버에서 Destroy되면 클라 화면에서도 사라져야 한다.
	bReplicates = true;

	// overlap 판정용 collision. 시각 mesh와 분리해서 나중에 mesh를 바꿔도 판정은 유지한다.
	PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
	SetRootComponent(PickupCollision);
	PickupCollision->InitSphereRadius(80.0f);
	PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupCollision->SetCollisionObjectType(ECC_WorldDynamic);
	PickupCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(PickupCollision);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWeaponPickup::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// claim 로직은 서버에서만 바인딩한다. 클라 overlap은 gameplay 결과를 결정하지 않는다.
		PickupCollision->OnComponentBeginOverlap.AddDynamic(this, &AWeaponPickup::OnPickupOverlap);
	}
}

void AWeaponPickup::OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 여러 플레이어가 동시에 닿아도 서버에서 한 번만 처리되게 막는다.
	if (!HasAuthority() || bClaimed)
	{
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(OtherActor);
	if (!Character || Character->IsDead())
	{
		return;
	}

	ClaimPickup_ServerOnly(Character);
}

void AWeaponPickup::ClaimPickup_ServerOnly(ABaseCharacter* Character)
{
	// "누가 아이템을 얻는가"는 gameplay state라 서버가 최종 결정한다.
	if (!HasAuthority() || bClaimed || !Character)
	{
		return;
	}

	if (!WeaponClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s 픽업 실패: WeaponClass 없음"), *GetName());
		return;
	}

	bClaimed = true;

	// 무기 actor도 서버에서 spawn해야 CurrentWeapon replication/Attach 흐름이 정상 동작한다.
	FActorSpawnParameters Params;
	Params.Owner = Character;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABaseWeapon* NewWeapon = GetWorld()->SpawnActor<ABaseWeapon>(
		WeaponClass,
		GetActorLocation(),
		GetActorRotation(),
		Params);

	if (!NewWeapon)
	{
		bClaimed = false;
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s 픽업 실패: weapon spawn 실패"), *GetName());
		return;
	}

	// EquipWeapon 안에서 기존 무기 제거 + 새 무기 attach + CurrentWeapon RepNotify 보강을 처리한다.
	Character->EquipWeapon(NewWeapon);

	UE_LOG(LogTemp, Warning, TEXT("[서버] %s 픽업: %s -> %s"),
		*GetName(), *Character->GetName(), *NewWeapon->GetName());

	Destroy();
}
