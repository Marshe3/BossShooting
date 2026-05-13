// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/AlienBase.h"

#include "Net/UnrealNetwork.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Enemy/AlienAIController.h"

// Sets default values
AAlienBase::AAlienBase()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false; // 적은 AI가 행동결정, 직접Tick 안씀
	
	// 멀티플레이
	bReplicates = true;
	SetReplicateMovement(true);
	
	// 회전 - AI 컨트롤러가 호ㅠㅣ전 방향 결정
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true; //AI 가 yaw 회전 제어
	bUseControllerRotationRoll = false;
	
	// CMC - 적은 이동 방향으로 자동 회전
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 400.0f; //러셔 기본 속도
	// 이 캐릭터가 사용할 AI 컨트롤러 클래스
	AIControllerClass = AAlienAIController::StaticClass();
	//레벨 배치 시 + 런타임 스폰시 모두 자동 빙의
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	
	CurrentHealth = MaxHealth;
}

// Called when the game starts or when spawned
void AAlienBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		CurrentHealth = MaxHealth;
	}
}

void AAlienBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAlienBase, CurrentHealth);
}

float AAlienBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || IsDead()) return 0.0f;
	
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	if (ActualDamage > 0.0f)
	{
		const float OldHealth = CurrentHealth;
		CurrentHealth = FMath::Max(0.0f, CurrentHealth - ActualDamage);
		
		OnRep_Health(OldHealth); // 서버 수동 호출
		
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s: %.1f -> %.1f (데미지 %.1f)"),
		*GetName(), OldHealth, CurrentHealth, ActualDamage);
		
		if (CurrentHealth <= 0.0f)
		{
			HandleDeath();
		}
	}
	
	return ActualDamage;
}

void AAlienBase::OnRep_Health(float OldHealth)
{
	const TCHAR* RoleStr = HasAuthority() ? TEXT("서버") : TEXT("클라");
	UE_LOG(LogTemp, Warning, TEXT("[%s] %s OnRep_Health: %.1f -> %.1f"),
		RoleStr, *GetName(), OldHealth, CurrentHealth);
}

void AAlienBase::HandleDeath()
{
	if (!HasAuthority()) return;
	
	UE_LOG(LogTemp, Warning, TEXT("[서버] %s 사망"), *GetName());
	
	// 모든 클라에 사망 연출 전파
	Multicast_OnDeath();
	
	// 3초 후 자동으로 액터 제거 (시체가 잠시 보이게)
	SetLifeSpan(3.0f);
}

// Called every frame
void AAlienBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AAlienBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AAlienBase::Multicast_OnDeath_Implementation()
{
	// 모든 클라(서버 포함)에서 실행됨
	
	// 캡슐 콜리전 끄기 - 시체가 플레이어 막지 않게;
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// CNC 정지
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	
	// 메쉬 ragdoll 전환 - 풀썩 쓰러지는 연출
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (MeshComp)
	{
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComp->SetSimulatePhysics(true);
	}
}