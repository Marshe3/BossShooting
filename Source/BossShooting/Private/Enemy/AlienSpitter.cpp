// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/AlienSpitter.h"

#include "Character/BaseCharacter.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapon/BaseProjectile.h"

AAlienSpitter::AAlienSpitter()
{
	// 스피터는 원거리 적이므로 기본 러셔 근접 공격 타이머는 꺼둔다.
	bCanMeleeAttack = false;
	MaxHealth = 50.0f;

	GetCharacterMovement()->MaxWalkSpeed = 260.0f;
}

void AAlienSpitter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// 원거리 공격 판단도 서버에서만 시작한다. 클라는 projectile/health replication 결과만 본다.
		StartRangedAttackTimer_ServerOnly();
	}
}

void AAlienSpitter::HandleDeath()
{
	if (HasAuthority())
	{
		StopRangedAttackTimer_ServerOnly();
	}

	Super::HandleDeath();
}

void AAlienSpitter::StartRangedAttackTimer_ServerOnly()
{
	if (!HasAuthority() || !bCanRangedAttack || RangedAttackCheckInterval <= 0.0f)
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		RangedAttackTimerHandle,
		this,
		&AAlienSpitter::TryRangedAttack_ServerOnly,
		RangedAttackCheckInterval,
		true);
}

void AAlienSpitter::StopRangedAttackTimer_ServerOnly()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RangedAttackTimerHandle);
	}
}

void AAlienSpitter::TryRangedAttack_ServerOnly()
{
	// target 선택, cooldown, projectile spawn 모두 서버 권위.
	if (!HasAuthority() || IsDead())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	if (CurrentTime - LastRangedAttackTime < RangedAttackCooldown)
	{
		return;
	}

	ABaseCharacter* Target = FindClosestRangedTarget_ServerOnly();
	if (!Target)
	{
		return;
	}

	LastRangedAttackTime = CurrentTime;
	SpawnAcidProjectile_ServerOnly(Target);
}

void AAlienSpitter::SpawnAcidProjectile_ServerOnly(ABaseCharacter* Target)
{
	if (!HasAuthority() || !Target)
	{
		return;
	}

	if (!AcidProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s 원거리 공격 실패: AcidProjectileClass 없음"), *GetName());
		return;
	}

	const FVector SpawnLocation =
		GetActorLocation()
		+ GetActorForwardVector() * ProjectileSpawnForwardOffset
		+ FVector(0.0f, 0.0f, ProjectileSpawnUpOffset);
	const FVector AimPoint = Target->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	const FVector AimDirection = (AimPoint - SpawnLocation).GetSafeNormal();

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABaseProjectile* Projectile = GetWorld()->SpawnActor<ABaseProjectile>(
		AcidProjectileClass,
		SpawnLocation,
		AimDirection.Rotation(),
		Params);

	if (!Projectile)
	{
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s 원거리 공격 실패: projectile spawn 실패"), *GetName());
		return;
	}

	// projectile 이동/충돌/데미지는 projectile actor가 서버에서 처리한다.
	Projectile->InitProjectile(AimDirection, GetController(), this, this);

	UE_LOG(LogTemp, Warning, TEXT("[서버] %s 산성 침 발사 -> %s"),
		*GetName(), *Target->GetName());
}

ABaseCharacter* AAlienSpitter::FindClosestRangedTarget_ServerOnly() const
{
	if (!HasAuthority())
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	ABaseCharacter* ClosestTarget = nullptr;
	float ClosestDistanceSq = FMath::Square(RangedAttackRange);
	const FVector SpitterLocation = GetActorLocation();

	for (TActorIterator<ABaseCharacter> It(World); It; ++It)
	{
		ABaseCharacter* Candidate = *It;
		if (!IsValid(Candidate) || Candidate->IsDead())
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared2D(SpitterLocation, Candidate->GetActorLocation());
		if (DistanceSq <= ClosestDistanceSq)
		{
			ClosestDistanceSq = DistanceSq;
			ClosestTarget = Candidate;
		}
	}

	return ClosestTarget;
}
