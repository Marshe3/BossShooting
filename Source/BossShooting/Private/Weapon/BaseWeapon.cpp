// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/BaseWeapon.h"

#include "Net/UnrealNetwork.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/BaseCharacter.h"

// Sets default values
ABaseWeapon::ABaseWeapon()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false; // 무기는 Tick 필요 없음 (이벤트 기반)
	
	// === 멀티플레이 기본 ===
	bReplicates = true;
	SetReplicateMovement(true); // 무기가 attach되면 zoflrxj EKfk dnawlrdladl ehdrlghk
	
	// === 메쉬 ===
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// attach 중일 땐 콜리전 무의미, 픽업으로 떠 있을 땐 별도 Pickup 액터에서 처리
	
	CurrentAmmo = MaxAmmo; // 컴파일 시점엔 BP 기본값 반영 전이라 BeginPlay에서 다시 세팅
	
	
	
}

// Called when the game starts or when spawned
void ABaseWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	// BP에서 정의된 MaxAmmo로 탄약 초기화 (서버에서만)
	if (HasAuthority())
	{
		CurrentAmmo = MaxAmmo;
	}
}

void ABaseWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ABaseWeapon, CurrentAmmo);
	DOREPLIFETIME(ABaseWeapon, OwningCharacter);
}

void ABaseWeapon::AttachToOwner(ABaseCharacter* NewOwner)
{
	// 서버에서만 호출되어야 함
	if (!HasAuthority() || !NewOwner) return;
	
	OwningCharacter = NewOwner;
	SetOwner(NewOwner);
	
	const FDetachmentTransformRules Rules(EDetachmentRule::KeepWorld, true);
	DetachFromActor(Rules);
	
	OnRep_OwningCharacter();
}

void ABaseWeapon::OnRep_OwningCharacter()
{
	// 서버/클라 양쪽에서 호출도미
	// attach 자체는 bReplicateMovement로 자동 동기화되니까
	// 여기서는 UI 갱신, 시각 효과 같은 부수 작업
	const TCHAR* RoleStr = HasAuthority() ? TEXT("서버") : TEXT("클라");
	const FString OwnerName = OwningCharacter ? OwningCharacter->GetName() : TEXT("None");

	UE_LOG(LogTemp, Warning, TEXT("[%s] %s OwningCharacter: %s"),
		RoleStr, *GetName(), *OwnerName);
}

void ABaseWeapon::OnRep_CurrentAmmo(int32 OldAmmo)
{
	const TCHAR* RoleStr = HasAuthority() ? TEXT("서버") : TEXT("클라");

	UE_LOG(LogTemp, Warning, TEXT("[%s] %s 탄약: %d -> %d"),
		RoleStr, *GetName(), OldAmmo, CurrentAmmo);
		// 추후 HUD 탄약 표시 갱신
		
}

// Called every frame
void ABaseWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseWeapon::DetachFromOwner()
{
	if (!HasAuthority()) return;

	OwningCharacter = nullptr;
	SetOwner(nullptr);

	const FDetachmentTransformRules Rules(EDetachmentRule::KeepWorld, true);
	DetachFromActor(Rules);

	OnRep_OwningCharacter();
}
