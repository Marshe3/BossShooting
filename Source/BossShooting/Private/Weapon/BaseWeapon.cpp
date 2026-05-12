// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/BaseWeapon.h"

#include "Net/UnrealNetwork.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/BaseCharacter.h"
#include "Engine/DamageEvents.h" //FPointDamageEvent
#include "DrawDebugHelpers.h" //DrawDebugLine 디버깅용
#include "EnhancedInputComponent.h"


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
	
	// 발사 시작점/방향 계산 (캐릭터 위치 + 캐릭터 정면 방향)
	const FVector TraceStart = OwningCharacter->GetActorLocation();
	const FVector TraceEnd = TraceStart + OwningCharacter->GetActorForwardVector() * Range;
	
	// 서버에 발사 요청
	Server_Fire(TraceStart, TraceEnd);
}

void ABaseWeapon::Server_Fire_Implementation(const FVector& TraceStart, const FVector& TraceEnd)
{
	// ===권위 검증===
	
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
	}
	
	// 검증 통과 -> 상태 갱신
	LastFireTime = CurrentTime;
	const int32 OldAmmo = CurrentAmmo;
	CurrentAmmo--;
	OnRep_CurrentAmmo(OldAmmo); // 서버에서 수동 호출 (RepNotify 패턴)
	
	// === 히트스캔 + 데미지 ===
	PerformLineTrace(TraceStart, TraceEnd);
}

void ABaseWeapon::PerformLineTrace(const FVector& TraceStart, const FVector& TraceEnd)
{
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this); // 무기 자기 자신 무시
	Params.AddIgnoredActor(OwningCharacter); // 쏘는 사람 무시
	Params.bTraceComplex = false; // 단순 콜리전만 (성능)
	
	const bool bHit = GetWorld() -> LineTraceSingleByChannel(
		Hit,TraceStart, TraceEnd, ECC_Pawn, Params);
	
	AActor* HitActor = nullptr;
	
	if (bHit && Hit.GetActor())
	{
		HitActor = Hit.GetActor();
		
		// 데미지 적용(서버 권위)
		FPointDamageEvent DmgEvent(Damage, Hit, (TraceEnd - TraceStart).GetSafeNormal(), nullptr);
		HitActor -> TakeDamage(
			Damage,
			DmgEvent,
			OwningCharacter ? OwningCharacter->GetController() : nullptr, this);
		
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s 히트: %s (데미지 %.1f)"),
			*GetName(), *HitActor->GetName(), Damage);
	}
	
	// === 전체 클라에 이펙트 전파 ===
	// bHit이면 실제 충돌 위치까지, 아니면 끝점까지 라인 그림
	const FVector FXEnd = bHit ? Hit.ImpactPoint : TraceEnd;
	Multicast_PlayFireFX(TraceStart, FXEnd, HitActor);
}

void ABaseWeapon::Multicast_PlayFireFX_Implementation(const FVector& TraceStart, const FVector& TraceEnd, AActor* HitActor)
{
	// 디버그 라인 - 5초간 보임
	DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Yellow, false, 5.0f, 0, 2.0f);
	
	if (HitActor)
	{
		// 맞은 점 표시
		DrawDebugSphere(GetWorld(), TraceEnd, 15.0f, 8, FColor::Red, false, 5.0f);
	}
	// 추후 Niagara 머즐 플래시, 사운드, 탄피 등 추가
}