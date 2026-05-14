// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyWaveTrigger.generated.h"

class AAlienBase;
class UBoxComponent;
class UPrimitiveComponent;

UCLASS()
class BOSSSHOOTING_API AEnemyWaveTrigger : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AEnemyWaveTrigger();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wave|Components")
	TObjectPtr<UBoxComponent> TriggerBox;
	
	// 서버가 wave 시작 시 spawn할 적 BP 클래스들 예 : BP_AlienRusher, BP_AlienSpitter
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "Wave|Spawn")
	TArray<TSubclassOf<AAlienBase>> EnemyClasses;
	
	// 비워두면 이 Trigger actor 위치 기준으로 스폰한다.
	// BP에서 TargetPoint 같은 actor들을 넣으면 그 위치에 순서대로 스폰한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave|Spawn")
	TArray<TObjectPtr<AActor>> SpawnPoints;
	
	// SpawnPoints보다 EnemyClassses가 많을 때, 같은 spawn point 주변에 살짝 흩뿌리는 거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave|Spawn", meta = (ClampMin = "0.0"))
	float SpawnScatterRadius = 120.0f;
	
	UFUNCTION()
	void OnTriggerOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
	
private:
	// 서버 내부 중복 실행 방지. 클라 UI가 읽는 값이 아니므로 지금은 replicate하지 않는다.
	bool bTriggered = false;
	
	void SpawnWave_ServerOnly();
	FTransform GetSpawnTransformForIndex(int32 Index) const;
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
