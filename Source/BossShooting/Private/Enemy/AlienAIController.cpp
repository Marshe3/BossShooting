// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/AlienAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

// Sets default values
AAlienAIController::AAlienAIController()
{
	// PlayerState 안 만듦 (AI니까 불필요, 메모리 절약)
	bWantsPlayerState = false;

}

// Called when the game starts or when spawned
void AAlienAIController::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAlienAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AAlienAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (BehaviorTreeAsset && BehaviorTreeAsset->BlackboardAsset)
	{
		// Blackboard 먼저 셋업(BT가 BB에 의존하므로)
		UBlackboardComponent* BlackboardComp = nullptr;
		UseBlackboard(BehaviorTreeAsset->BlackboardAsset, BlackboardComp);
		//BT 실행
		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void AAlienAIController::OnUnPossess()
{
	if (UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent))
	{
		BTComp->StopTree();
	}
	Super::OnUnPossess();
}