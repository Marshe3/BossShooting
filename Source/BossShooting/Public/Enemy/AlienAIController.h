// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AlienAIController.generated.h"

class UBehaviorTree;

UCLASS()
class BOSSSHOOTING_API AAlienAIController : public AAIController
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AAlienAIController();
	
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// BP에서 BT 자산 할당
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
