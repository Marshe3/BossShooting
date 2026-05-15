// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BossQueen.h"

#include "Net/UnrealNetwork.h"
#include "Character/BaseCharacter.h"
#include "Weapon/BaseProjectile.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Enemy/AlienBase.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Engine/DamageEvents.h"

// Sets default values
ABossQueen::ABossQueen()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	MaxHealth = 600.0f;
	OriginalWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	bCanMeleeAttack = false;
}

// Called when the game starts or when spawned
void ABossQueen::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		// 보스 페이즈와 공격 패턴 판단은 서버가 결정하고, 클라는 복제 결과를 본다.
		CurrentPhase = EBossQueenPhase::Phase1;
		bIsInvincible = false;
		bPhase2SummonsStarted = false;
		Phase2SpawnedMinions.Reset();
		StartBossAttackTimer_ServerOnly();
	}
}

void ABossQueen::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ABossQueen, CurrentPhase);
	DOREPLIFETIME(ABossQueen, bIsInvincible);
}

float ABossQueen::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (!HasAuthority() || IsDead())
	{
		return 0.0f;
	}

	if (bIsInvincible)
	{
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s 데미지 무시: 무적 상태"), *GetName());
		return 0.0f;
	}
	
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	if (ActualDamage > 0.0f && !IsDead())
	{
		// 체력은 부모 AAlienBase가 깎고, 보스는 그 결과를 보고 phase만 판단
		UpdatePhase_ServerOnly();
	}
	return ActualDamage;
}

void ABossQueen::HandleDeath()
{
	if (HasAuthority())
	{
		StopBossAttackTimer_ServerOnly();
		
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(Phase2MinionCheckTimerHandle);
		}
		
		SetPhase_ServerOnly(EBossQueenPhase::Dead);
	}
	
	Super::HandleDeath();
}

void ABossQueen::UpdatePhase_ServerOnly()
{
	if (!HasAuthority() || IsDead())
	{
		return;
	}
	
	const float HealthRatio = MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
	
	if (HealthRatio <= Phase3HealthRatio)
	{
		SetPhase_ServerOnly(EBossQueenPhase::Phase3);
	}
	else if (HealthRatio <= Phase2HealthRatio)
	{
		SetPhase_ServerOnly(EBossQueenPhase::Phase2);
	}
	else
	{
		SetPhase_ServerOnly(EBossQueenPhase::Phase1);
	}
}

void ABossQueen::SetPhase_ServerOnly(EBossQueenPhase NewPhase)
{
	// 페이즈 변경은 서버만 결정한다. 이미 같은 페이즈면 중복 처리하지 않는다.
	if (!HasAuthority() || CurrentPhase == NewPhase)
	{
		return;
	}
	// OnRep에서 이전/현재 페이즈를 비교할 수 있도록 변경 전 값을 저장한다.
	const EBossQueenPhase OldPhase = CurrentPhase;
	CurrentPhase = NewPhase;
	
	// 서버에서도 RepNotify와 같은 처리를 직접 호출한다.
	OnRep_CurrentPhase(OldPhase);
	
	// 지금은 Phase1 공격 패턴만 있으므로 Phase1일 때만 보스 공격 타이머를 켠다.
	if (CurrentPhase == EBossQueenPhase::Phase1)
	{
		StartBossAttackTimer_ServerOnly();
	}
	else
	{
		StopBossAttackTimer_ServerOnly();
	}
	
	if (CurrentPhase == EBossQueenPhase::Phase2)
	{
		EnterPhase2_ServerOnly();
	}
	
	if (CurrentPhase == EBossQueenPhase::Phase3)
	{
		EnterPhase3_ServerOnly();
	}
}

void ABossQueen::EnterPhase2_ServerOnly()
{
	// Phase2 진입 처리는 서버에서 한 번만 실행한다.
	if (!HasAuthority() || IsDead() || bPhase2SummonsStarted)
	{
		return;
	}
	
	bPhase2SummonsStarted = true;
	bIsInvincible =true;
	
	UE_LOG(LogTemp, Warning, TEXT("[서버] %s Phase2 진입 : 무적 활성화 + 잡몹 소환 시작"), *GetName());
	
	SpawnPhase2Minions_ServerOnly();
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			Phase2MinionCheckTimerHandle,
			this,
			&ABossQueen::CheckPhase2Minions_ServerOnly,
			Phase2MinionCheckInterval,
			true);
	}
}

void ABossQueen::ExitPhase2Invincibility_ServerOnly()
{
	// 소환몹이 정리되면 서버가 무적을 해제한다. 클라는 bIsInvincible 복제 결과를 본다.
	if (!HasAuthority() || IsDead())
	{
		return;
	}
	
	bIsInvincible = false;
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Phase2MinionCheckTimerHandle);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[서버] %s Phase2 소환몹 정리 완료: 무적 해제"), *GetName());
}

void ABossQueen::SpawnPhase2Minions_ServerOnly()
{
	// Phase2 소환도 서버가 결정한다. 클라는 spawn된 replicated enemy들을 보게 된다.
	if (!HasAuthority() || IsDead())
	{
		return;
	}
	
	Phase2SpawnedMinions.Reset();
	
	if (Phase2SummonClasses.Num() == 0 || Phase2SummonCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s Phase2 소환 클래스가 없어서 무적을 바로 해제"), *GetName());
		ExitPhase2Invincibility_ServerOnly();
		return;
	}
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	const FVector BossLocation = GetActorLocation();
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride =  ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	for (int32 Index = 0 ; Index < Phase2SummonCount; ++Index)
	{
		const int32 ClassIndex = FMath::RandRange(0, Phase2SummonClasses.Num() - 1);
		TSubclassOf<AAlienBase> MinionClass = Phase2SummonClasses[ClassIndex];
		
		if (!MinionClass)
		{
			continue;
		}
		
		const float AngleDegrees = (360.0f / FMath::Max(1, Phase2SummonCount)) * Index;
		const FVector Direction = FVector(1.0f, 0.0f, 0.0f).RotateAngleAxis(AngleDegrees, FVector::UpVector);
		const FVector SpawnLocation = BossLocation + Direction * Phase2SummonRadius;
		const FRotator SpawnRotation = Direction.Rotation();
		
		AAlienBase* SpawnedMinion = World ->SpawnActor<AAlienBase>(
			MinionClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParams);
		
		if (!SpawnedMinion)
		{
			continue;
		}
		
		Phase2SpawnedMinions.Add(SpawnedMinion);
		
		UE_LOG(LogTemp,Warning, TEXT("[서버] %s Phase2 잡몹 소환: %s"),
			*GetName(),
			*SpawnedMinion->GetName());
	}
	
	if (Phase2SpawnedMinions.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s Phase2 소환 성공한 잡몹이 없어서 무적을 바로 해제"), *GetName());
		ExitPhase2Invincibility_ServerOnly();
	}
}

void ABossQueen::CheckPhase2Minions_ServerOnly()
{
	// 서버가 소환몹 생존 여부를 확인한다. 다 정리되면 보스 무적을 해제한다.
	if (!HasAuthority() || IsDead())
	{
		return;
	}

	for (int32 Index = Phase2SpawnedMinions.Num() - 1; Index >= 0; --Index)
	{
		AAlienBase* Minion = Phase2SpawnedMinions[Index];
		if (!IsValid(Minion) || Minion->IsDead())
		{
			Phase2SpawnedMinions.RemoveAtSwap(Index);
		}
	}

	if (Phase2SpawnedMinions.Num() > 0)
	{
		return;
	}

	ExitPhase2Invincibility_ServerOnly();
}

void ABossQueen::EnterPhase3_ServerOnly()
{
	// Phase3 격노 전환도 서버가 결정한다. 이동 속도와 공격 타이머도 서버 기준으로 바꾼다.
	if (!HasAuthority() || IsDead())
	{
		return;
	}
	
	bIsInvincible = false;
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Phase2MinionCheckTimerHandle);
	}
	
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->MaxWalkSpeed = Phase3WalkSpeed;
	}
	
	StartPhase3AttackTimer_ServerOnly();
	
	UE_LOG(LogTemp, Warning, TEXT("[서버] %s Phase3 격노 진입: 속도 %.1f -> %.1f"),
		*GetName(),
		OriginalWalkSpeed,
		Phase3WalkSpeed);
}

void ABossQueen::StartPhase3AttackTimer_ServerOnly()
{
	// Phase3에서는 같은 공격 타이머 핸들을 더 짧은 주기로 다시 등록한다.
	if (!HasAuthority() || IsDead() || CurrentPhase != EBossQueenPhase::Phase3)
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if (!World || Phase3AttackInterval <= 0.0f)
	{
		return;
	}
	
	World->GetTimerManager().SetTimer(
		BossAttackTimerHandle,
		this,
		&ABossQueen::TryPhase3Attack_ServerOnly,
		Phase3AttackInterval,
		true);
}

void ABossQueen::TryPhase3Attack_ServerOnly()
{
	// Phase3 격노 공격도 서버가 선택한다. 클라는 데미지/projectile 복제 결과를 본다.
	if (!HasAuthority() || IsDead() || CurrentPhase != EBossQueenPhase::Phase3)
	{
		return;
	}

	ABaseCharacter* Target = FindClosestLivingPlayer_ServerOnly();
	if (!Target)
	{
		return;
	}

	FaceTarget_ServerOnly(Target);

	const float DistanceSq = FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation());
	if (DistanceSq <= FMath::Square(Phase3MeleeRange))
	{
		DoPhase3MeleeSweep_ServerOnly();
	}
	else
	{
		DoPhase3ProjectileBurst_ServerOnly(Target);
	}
}

void ABossQueen::DoPhase3MeleeSweep_ServerOnly()
{
	// Phase3 근접 공격도 서버가 판정한다. Phase1보다 범위와 데미지가 강하다.
	if (!HasAuthority() || IsDead())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector BossLocation = GetActorLocation();
	const FVector BossForward = GetActorForwardVector().GetSafeNormal2D();
	const float RangeSq = FMath::Square(Phase3MeleeRange);
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(Phase1MeleeHalfAngleDegrees));

	int32 HitCount = 0;

	for (TActorIterator<ABaseCharacter> It(World); It; ++It)
	{
		ABaseCharacter* Candidate = *It;
		if (!IsValid(Candidate) || Candidate->IsDead())
		{
			continue;
		}

		const FVector ToTarget = Candidate->GetActorLocation() - BossLocation;
		const float DistanceSq = FVector::DistSquared2D(BossLocation, Candidate->GetActorLocation());
		if (DistanceSq > RangeSq)
		{
			continue;
		}

		const FVector DirectionToTarget = ToTarget.GetSafeNormal2D();
		const float ForwardDot = FVector::DotProduct(BossForward, DirectionToTarget);
		if (ForwardDot < MinDot)
		{
			continue;
		}

		FDamageEvent DamageEvent;
		const float ActualDamage = Candidate->TakeDamage(
			Phase3MeleeDamage,
			DamageEvent,
			GetController(),
			this);

		++HitCount;

		UE_LOG(LogTemp, Warning, TEXT("[서버] %s Phase3 격노 근접 -> %s (요청 %.1f / 적용 %.1f)"),
			*GetName(),
			*Candidate->GetName(),
			Phase3MeleeDamage,
			ActualDamage);
	}

	UE_LOG(LogTemp, Warning, TEXT("[서버] %s Phase3 격노 근접 완료: %d명 명중"),
		*GetName(),
		HitCount);
}

void ABossQueen::DoPhase3ProjectileBurst_ServerOnly(ABaseCharacter* Target)
{
	// Phase3 projectile도 서버가 spawn한다. 클라는 replicated projectile들을 본다.
	if (!HasAuthority() || IsDead() || !IsValid(Target) || Target->IsDead())
	{
		return;
	}
	
	if (!Phase1ProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s Phase1ProjectileClass가 비어 있어서 Phase3 발사도 하지 않음"), *GetName());
		return;
	}
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	const FVector BossLocation = GetActorLocation();
	FVector AimDirection = Target->GetActorLocation() - BossLocation;
	AimDirection.Z = 0.0f;
	
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = GetActorForwardVector();
	}
	
	AimDirection = AimDirection.GetSafeNormal();
	
	const FVector SpawnLocation = BossLocation + AimDirection * Phase1ProjectileSpawnForwardOffset;
	const int32 SafeProjectileCount = FMath::Max(1, Phase3ProjectileCount);
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (int32 Index = 0; Index < SafeProjectileCount; ++Index)
	{
		float Alpha = 0.5f;
		if (SafeProjectileCount > 1)
		{
			Alpha = static_cast<float>(Index) / static_cast<float>(SafeProjectileCount - 1);
		}

		const float SpreadAngle = FMath::Lerp(
			-Phase3ProjectileTotalSpreadDegrees * 0.5f,
			Phase3ProjectileTotalSpreadDegrees * 0.5f,
			Alpha);

		const FVector ShotDirection = AimDirection.RotateAngleAxis(SpreadAngle, FVector::UpVector).GetSafeNormal();
		const FRotator ShotRotation = ShotDirection.Rotation();

		ABaseProjectile* Projectile = World->SpawnActor<ABaseProjectile>(
			Phase1ProjectileClass,
			SpawnLocation,
			ShotRotation,
			SpawnParams);

		if (!Projectile)
		{
			continue;
		}

		Projectile->InitProjectile(ShotDirection, GetController(), this, this);
	}

	UE_LOG(LogTemp, Warning, TEXT("[서버] %s Phase3 격노 탄막 발사: %d발 -> %s"),
		*GetName(),
		SafeProjectileCount,
		*Target->GetName());
}

void ABossQueen::OnRep_CurrentPhase(EBossQueenPhase OldPhase)
{
	const TCHAR* RoleStr = HasAuthority() ? TEXT("서버") : TEXT("클라");
	
	UE_LOG(LogTemp, Warning, TEXT("[%s] %s 보스 페이즈 변경 : %d -> %d"),
		RoleStr,
		*GetName(),
		static_cast<int32>(OldPhase),
		static_cast<int32>(CurrentPhase));
	
	
}

void ABossQueen::StartBossAttackTimer_ServerOnly()
{
	// 보스 공격 패턴 선택은 서버에서만 돈다. 클라는 결과로 생긴 데미지/projectile만 본다.
	if (!HasAuthority() || IsDead() || CurrentPhase != EBossQueenPhase::Phase1)
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if (!World || Phase1AttackInterval <= 0.0f)
	{
		return;
	}
	
	World->GetTimerManager().SetTimer(
		BossAttackTimerHandle,
		this,
		&ABossQueen::TryPhase1Attack_ServerOnly,
		Phase1AttackInterval,
		true);
}

void ABossQueen::StopBossAttackTimer_ServerOnly()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BossAttackTimerHandle);
	}
	
	
}

void ABossQueen::TryPhase1Attack_ServerOnly()
{
	// Phase1 공격 선택은 서버 권위. 타겟 거리만 보고 melee/projectile 중 하나를 고른다
	if (!HasAuthority() || IsDead() || CurrentPhase != EBossQueenPhase::Phase1)
	{
		return;
	}
	
	ABaseCharacter* Target = FindClosestLivingPlayer_ServerOnly();
	if (!Target)
	{
		return;
	}
	
	FaceTarget_ServerOnly(Target);
	
	const float DistanceSq = FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation());
	if (DistanceSq <= FMath::Square(Phase1MeleeRange))
	{
		DoPhase1MeleeSweep_ServerOnly();
	}
	else
	{
		DoPhase1ProjectileBurst_ServerOnly(Target);
	}
}

void ABossQueen::FaceTarget_ServerOnly(ABaseCharacter* Target)
{
	// 공격 방향도 서버가 정한다. 회전은 movement replication으로 클라에 보인다.
	if (!HasAuthority() || !IsValid(Target))
	{
		return;
	}
	
	FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.0f;
	
	if (ToTarget.IsNearlyZero())
	{
		return;
	}
	
	SetActorRotation(ToTarget.Rotation());
}

ABaseCharacter* ABossQueen::FindClosestLivingPlayer_ServerOnly() const
{
	// 서버가 보스 공격 타겟을 고른다. 죽은 플레이어는 타겟에서 제외한다.
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
	float ClosestDistanceSq = TNumericLimits<float>::Max();
	const FVector BossLocation = GetActorLocation();
	
	for (TActorIterator<ABaseCharacter> It(World); It; ++It)
	{
		ABaseCharacter* Candidate = *It;
		if (!IsValid(Candidate) || Candidate->IsDead())
		{
			continue;
		}
		
		const float DistanceSq = FVector::DistSquared2D(BossLocation, Candidate->GetActorLocation());
		if (DistanceSq < ClosestDistanceSq)
		{
			ClosestDistanceSq = DistanceSq;
			ClosestTarget = Candidate;
		}
	}
	
	return ClosestTarget;
}

void ABossQueen::DoPhase1MeleeSweep_ServerOnly()
{
	// 근접 휘두르기 판정도 서버가 한다. 클라는 첼격 복제 결과를 맞았는지 알게 된다.
	if (!HasAuthority() || IsDead())
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	const FVector BossLocation = GetActorLocation();
	const FVector BossForward = GetActorForwardVector().GetSafeNormal2D();
	const float RangeSq = FMath::Square(Phase1MeleeRange);
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(Phase1MeleeHalfAngleDegrees));
	
	int32 HitCount = 0;
	
	for (TActorIterator<ABaseCharacter> It(World); It; ++It)
	{
		ABaseCharacter* Candidate = *It;
		if (!IsValid(Candidate) || Candidate->IsDead())
		{
			continue;
		}
		
		const FVector ToTarget = Candidate->GetActorLocation() - BossLocation;
		const float DistanceSq = FVector::DistSquared2D(BossLocation, Candidate->GetActorLocation());
		if (DistanceSq > RangeSq)
		{
			continue;
		}
		
		const FVector DirectionToTarget = ToTarget.GetSafeNormal2D();
		const float ForwardDot = FVector::DotProduct(BossForward, DirectionToTarget);
		if (ForwardDot < MinDot)
		{
			continue;
		}
		
		FDamageEvent DamageEvent;
		const float ActualDamage = Candidate->TakeDamage(
			Phase1MeleeDamage,
			DamageEvent,
			GetController(),
			this);
		
		++HitCount;
		
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s Phase1 근접 휘두르기 -> %s (요청 %.1f / 적용 %.1f)"),
			*GetName(),
			*Candidate->GetName(),
			Phase1MeleeDamage,
			ActualDamage);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[서버] %s Phase1 근접 휘두르기 완료 : %d명 명중"),
		*GetName(),
		HitCount);
}

void ABossQueen::DoPhase1ProjectileBurst_ServerOnly(ABaseCharacter* Target)
{
	// projectile spawn도 서버 권위. 클라는 replicated projectile 이동과 폭발 FX를 본다.
	if (!HasAuthority() || IsDead() || !IsValid(Target) || Target->IsDead())
	{
		return;
	}

	if (!Phase1ProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s Phase1ProjectileClass가 비어 있어서 발사하지 않음"), *GetName());
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector BossLocation = GetActorLocation();
	FVector AimDirection = Target->GetActorLocation() - BossLocation;
	AimDirection.Z = 0.0f;

	if (AimDirection.IsNearlyZero())
	{
		AimDirection = GetActorForwardVector();
	}

	AimDirection = AimDirection.GetSafeNormal();

	const FVector SpawnLocation = BossLocation + AimDirection * Phase1ProjectileSpawnForwardOffset;

	const float SpreadAngles[3] =
	{
		-Phase1ProjectileSpreadDegrees,
		0.0f,
		Phase1ProjectileSpreadDegrees
	};

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (float SpreadAngle : SpreadAngles)
	{
		const FVector ShotDirection = AimDirection.RotateAngleAxis(SpreadAngle, FVector::UpVector).GetSafeNormal();
		const FRotator ShotRotation = ShotDirection.Rotation();

		ABaseProjectile* Projectile = World->SpawnActor<ABaseProjectile>(
			Phase1ProjectileClass,
			SpawnLocation,
			ShotRotation,
			SpawnParams);

		if (!Projectile)
		{
			continue;
		}

		Projectile->InitProjectile(ShotDirection, GetController(), this, this);
	}

	UE_LOG(LogTemp, Warning, TEXT("[서버] %s Phase1 산성 3방향 발사 -> %s"),
		*GetName(),
		*Target->GetName());
	
}
