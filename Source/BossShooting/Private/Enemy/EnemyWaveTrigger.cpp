// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/EnemyWaveTrigger.h"

#include "Character/BaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Enemy/AlienBase.h"

// Sets default values
AEnemyWaveTrigger::AEnemyWaveTrigger()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	// 트리거 actor가 서버에서만 wave를 실행하더라도,
	// 에디터/멀티 환경에서 존재 자체는 네트워크 actor로 맞춰둔다.
	bReplicates = true;
	
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	
	TriggerBox->SetBoxExtent(FVector(300.0f, 300.0f, 150.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

// Called when the game starts or when spawned
void AEnemyWaveTrigger::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		// wave 시작 판정은 서버에서만 한다.
		// 클라 overlap으로 적을 spawn하면 화면마다 다른 결과가 생길 수 잇다.
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AEnemyWaveTrigger::OnTriggerOverlap);
	}
	
}

void AEnemyWaveTrigger::OnTriggerOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	// 서버에서 한번만 wave를 시작한다.
	if (!HasAuthority() || bTriggered)
	{
		return;
	}
		
	ABaseCharacter* Character = Cast<ABaseCharacter>(OtherActor);
	if (!Character || Character->IsDead())
	{
		return;
	}
	
	bTriggered = true;
	
	UE_LOG(LogTemp, Warning, TEXT("[서버] %s 웨이브 트리거 시작: %s"),
		*GetName(), *Character->GetName());
	
	SpawnWave_ServerOnly();
}

// Called every frame
void AEnemyWaveTrigger::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AEnemyWaveTrigger::SpawnWave_ServerOnly()
{
	// 실제 enemy spawn은 서버만 한다.
	if (!HasAuthority())
	{
		return;
	}
	
	if (EnemyClasses.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[서버] %s 웨이브 스폰 실패: EnemyClasses 비어 있음"), *GetName());
		return;
	}
	
	for (int32 Index = 0; Index < EnemyClasses.Num(); ++Index)
	{
		TSubclassOf<AAlienBase> EnemyClass = EnemyClasses[Index];
		if (!EnemyClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("[서버] %s 웨이브 스폰 건너뜀 : EnemyClasses[%d] 비어 있음"),
				*GetName(), Index);
			continue;
		}
		
		const FTransform SpawnTransform = GetSpawnTransformForIndex(Index);
		
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		
		AAlienBase* SpawnedEnemy = GetWorld()->SpawnActor<AAlienBase>(
			EnemyClass,
			SpawnTransform,
			Params);  
		
		if (SpawnedEnemy)
		{
			UE_LOG(LogTemp, Warning, TEXT("[서버] %s 웨이브 스폰 : %s"),
				*GetName(), *SpawnedEnemy->GetName());
		}
	}
}

FTransform AEnemyWaveTrigger::GetSpawnTransformForIndex(int32 Index) const
{
	const AActor* SpawnPoint = SpawnPoints.IsValidIndex(Index) ? SpawnPoints[Index] : nullptr;
	
	FTransform SpawnTransform = SpawnPoint ? SpawnPoint->GetActorTransform() : GetActorTransform();
	
	if (SpawnScatterRadius > 0.0f)
	{
		const FVector RandomOffset = FVector(
			FMath::FRandRange(-SpawnScatterRadius, SpawnScatterRadius),
			FMath::FRandRange(-SpawnScatterRadius, SpawnScatterRadius),
			0.0f);
		
		SpawnTransform.AddToTranslation(RandomOffset);
	}
	
	return SpawnTransform;
}
