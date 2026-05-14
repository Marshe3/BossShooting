// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/BaseWeapon.h"

#include "Net/UnrealNetwork.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/BaseCharacter.h"
#include "Engine/DamageEvents.h" //FPointDamageEvent
#include "DrawDebugHelpers.h" //DrawDebugLine 디버깅용
#include "EnhancedInputComponent.h"
#include "Weapon/BaseProjectile.h"


// Sets default values
ABaseWeapon::ABaseWeapon()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false; // 무기는 Tick 필요 없음 (이벤트 기반)
	
	// === 멀티플레이 기본 ===
	bReplicates = true;
	SetReplicateMovement(true); // 무기가 attach되거나 월드에 있을 때 위치/회전 결과를 클라에 보여준다.
	
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
	if (!HasAuthority() || !NewOwner)
	{
		const TCHAR* OwnerStr = NewOwner ? TEXT("OK") : TEXT("NULL");
		UE_LOG(LogTemp, Warning, TEXT("[Attach] 진입 실패: HasAuthority=%d, NewOwner=%s"),
			HasAuthority() ? 1 : 0, OwnerStr);
		return;
	}

	OwningCharacter = NewOwner;
	SetOwner(NewOwner);

	USkeletalMeshComponent* CharMesh = NewOwner->GetMesh();
	if (!CharMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Attach] CharMesh NULL — 캐릭터 메쉬가 없음"));
		return;
	}

	const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, true);
	const bool bAttached = AttachToComponent(CharMesh, Rules, AttachSocketName);

	const FVector WeaponLoc = GetActorLocation();
	const FVector SocketLoc = CharMesh->GetSocketLocation(AttachSocketName);
	const AActor* AttachParent = GetAttachParentActor();
	const FString ParentStr = AttachParent ? AttachParent->GetName() : TEXT("NULL");

	UE_LOG(LogTemp, Warning, TEXT("[Attach] Socket=%s, Result=%d, Parent=%s"),
		*AttachSocketName.ToString(), bAttached ? 1 : 0, *ParentStr);

	UE_LOG(LogTemp, Warning, TEXT("[Attach] WeaponLoc=%s / SocketLoc=%s"),
		*WeaponLoc.ToString(), *SocketLoc.ToString());

	OnRep_OwningCharacter();
}

void ABaseWeapon::OnRep_OwningCharacter()
{
	// ★ 클라이언트 측에서도 명시적으로 attach 수행
	//   서버는 AttachToOwner에서 이미 처리했지만, attach 정보의 자동 리플리케이션이
	//   timing 문제로 누락될 수 있어서 RepNotify에서 보강
	if (!HasAuthority() && OwningCharacter)
	{
		USkeletalMeshComponent* CharMesh = OwningCharacter->GetMesh();
		if (CharMesh)
		{
			const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, true);
			AttachToComponent(CharMesh, Rules, AttachSocketName);
		}
	}

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

void ABaseWeapon::StartFire()
{
	// 소유 캐릭터가 없으면 발사 불가
	if (!OwningCharacter) return;
	
	// 서버에 발사 방향만 요청한다. 실제 TraceStart와 pellet spread는 서버에서 계산한다.
	Server_Fire(OwningCharacter->GetActorForwardVector());
}

void ABaseWeapon::Server_Fire_Implementation(const FVector& AimDirection)
{
	// === 서버 권위 검증 ===
	// 여기서부터는 서버가 발사 가능 여부를 최종 결정한다.
	// 클라는 방향만 요청했고, 탄약/쿨다운/실제 판정은 신뢰하지 않는다.
	if (!OwningCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s 발사 거부: 소유 캐릭터 없음"), *GetName());
		return;
	}
	
	// 발사 쿨다운
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastFireTime < FireRate)
	{
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s 발사 거부: 쿨다운"), *GetName());
		return;
	}
	
	if (CurrentAmmo <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s 발사 거부: 탄약 없음"), *GetName());
		return;
	}
	
	// 검증 통과 -> 상태 갱신
	LastFireTime = CurrentTime;
	const int32 OldAmmo = CurrentAmmo;
	CurrentAmmo--;
	OnRep_CurrentAmmo(OldAmmo); // 서버에서 수동 호출 (RepNotify 패턴)
	
	// 클라가 보낸 방향이 0이면 서버가 알고 있는 캐릭터 정면으로 보정한다.
	const FVector SafeAimDirection = AimDirection.IsNearlyZero()
		? OwningCharacter->GetActorForwardVector()
		: AimDirection.GetSafeNormal();

	// 같은 Server_Fire 요청을 쓰되, 무기 BP의 FireMode에 따라 서버 판정 방식만 갈라진다.
	if (FireMode == EWeaponFireMode::Projectile)
	{
		SpawnProjectile_ServerOnly(SafeAimDirection);
	}
	else
	{
		const FVector TraceStart = OwningCharacter->GetActorLocation();
		PerformHitScanFire(TraceStart, SafeAimDirection);
	}
}

void ABaseWeapon::PerformHitScanFire(const FVector& TraceStart, const FVector& AimDirection)
{
	// HitScan은 서버가 즉시 trace를 쏘고, 맞은 Actor에게 바로 TakeDamage를 호출한다.
	// Shotgun도 같은 함수에서 pellet 개수와 spread만 늘려 처리한다.
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this); // 무기 자기 자신 무시
	Params.AddIgnoredActor(OwningCharacter); // 쏘는 사람 무시
	Params.bTraceComplex = false; // 단순 콜리전만 (성능)

	const int32 SafePelletCount = FMath::Max(1, PelletCount);
	const float SpreadRadians = FMath::DegreesToRadians(FMath::Max(0.0f, SpreadAngleDegrees));
	TArray<FVector> TraceEnds;
	TArray<AActor*> HitActors;
	TraceEnds.Reserve(SafePelletCount);
	HitActors.Reserve(SafePelletCount);

	for (int32 PelletIndex = 0; PelletIndex < SafePelletCount; ++PelletIndex)
	{
		// spread 랜덤도 서버에서 만든다. 클라가 pellet 방향을 정하면 hit 조작 여지가 생긴다.
		const FVector PelletDirection = (SafePelletCount > 1 && SpreadRadians > 0.0f)
			? FMath::VRandCone(AimDirection, SpreadRadians)
			: AimDirection;
		const FVector TraceEnd = TraceStart + PelletDirection * Range;
		
		FHitResult Hit;
		const bool bHit = GetWorld()->LineTraceSingleByChannel(
			Hit, TraceStart, TraceEnd, ECC_Pawn, Params);

		AActor* HitActor = nullptr;
		const FVector FXEnd = bHit ? Hit.ImpactPoint : TraceEnd;
		
		if (bHit && Hit.GetActor())
		{
			HitActor = Hit.GetActor();
			
			// 데미지 적용(서버 권위)
			FPointDamageEvent DmgEvent(Damage, Hit, PelletDirection, nullptr);
			HitActor->TakeDamage(
				Damage,
				DmgEvent,
				OwningCharacter ? OwningCharacter->GetController() : nullptr,
				this);
			
			UE_LOG(LogTemp, Warning, TEXT("[서버] %s 히트: %s (펠릿 %d/%d, 데미지 %.1f)"),
				*GetName(), *HitActor->GetName(), PelletIndex + 1, SafePelletCount, Damage);
		}

		TraceEnds.Add(FXEnd);
		HitActors.Add(HitActor);
	}
	
	// === 전체 클라에 이펙트 전파 ===
	Multicast_PlayFireFX(TraceStart, TraceEnds, HitActors);
}

void ABaseWeapon::SpawnProjectile_ServerOnly(const FVector& AimDirection)
{
	// Projectile은 Actor로 존재하므로 반드시 서버에서 spawn해야 한다.
	// 클라이언트는 replicated movement로 날아가는 결과를 본다.
	if (!HasAuthority() || !OwningCharacter)
	{
		return;
	}

	if (!ProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s 발사 거부: ProjectileClass 없음"), *GetName());
		return;
	}

	const FVector SafeAimDirection = AimDirection.IsNearlyZero()
		? OwningCharacter->GetActorForwardVector()
		: AimDirection.GetSafeNormal();
	const FVector SpawnLocation = OwningCharacter->GetActorLocation() + SafeAimDirection * ProjectileSpawnForwardOffset;
	const FRotator SpawnRotation = SafeAimDirection.Rotation();

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = OwningCharacter;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABaseProjectile* Projectile = GetWorld()->SpawnActor<ABaseProjectile>(
		ProjectileClass,
		SpawnLocation,
		SpawnRotation,
		Params);

	if (!Projectile)
	{
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s 발사 실패: Projectile spawn 실패"), *GetName());
		return;
	}

	// spawn 직후 발사 방향/instigator/damage causer를 넘겨 projectile 쪽 서버 판정에 사용한다.
	Projectile->InitProjectile(SafeAimDirection, OwningCharacter->GetController(), this, OwningCharacter);

	UE_LOG(LogTemp, Warning, TEXT("[서버] %s 프로젝타일 발사: %s"),
		*GetName(), *Projectile->GetName());
}

void ABaseWeapon::Multicast_PlayFireFX_Implementation(const FVector& TraceStart, const TArray<FVector>& TraceEnds, const TArray<AActor*>& HitActors)
{
	// 모든 클라(서버 포함)에서 실행되는 시각화용 RPC.
	// 맞았는지/데미지가 얼마인지는 이미 서버가 결정했고, 여기서는 debug line만 그린다.
	for (int32 Index = 0; Index < TraceEnds.Num(); ++Index)
	{
		const FVector& TraceEnd = TraceEnds[Index];
		AActor* HitActor = HitActors.IsValidIndex(Index) ? HitActors[Index] : nullptr;

		// 디버그 라인 - 5초간 보임
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Yellow, false, 5.0f, 0, 2.0f);
	
		if (HitActor)
		{
			// 맞은 점 표시
			DrawDebugSphere(GetWorld(), TraceEnd, 15.0f, 8, FColor::Red, false, 5.0f);
		}
	}
	// 추후 Niagara 머즐 플래시, 사운드, 탄피 등 추가
}
