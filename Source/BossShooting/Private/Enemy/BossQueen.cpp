// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BossQueen.h"

#include "Net/UnrealNetwork.h"


// Sets default values
ABossQueen::ABossQueen()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	MaxHealth = 600.0f;
	bCanMeleeAttack = false;
}

// Called when the game starts or when spawned
void ABossQueen::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		// 보스 페이즈는 서버가 HP 기준으로 결정하고 클라가 복제 결과를 본다.
		CurrentPhase = EBossQueenPhase::Phase1;
		bIsInvincible = false;
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
	if (!HasAuthority() || CurrentPhase == NewPhase)
	{
		return;
	}
	
	const EBossQueenPhase OldPhase = CurrentPhase;
	CurrentPhase = NewPhase;
	
	// 서버에서도 RepNotify와 같은 처리를 직접 호출한다.
	OnRep_CurrentPhase(OldPhase);
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
