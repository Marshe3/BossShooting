#include "Enemy/BTTask_FindClosestPlayer.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/BaseCharacter.h"
#include "EngineUtils.h"

UBTTask_FindClosestPlayer::UBTTask_FindClosestPlayer()
{
	NodeName = TEXT("Find Closest Player");
	BlackboardKey.SelectedKeyName = TEXT("TargetActor");
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindClosestPlayer, BlackboardKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_FindClosestPlayer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	const APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn)
	{
		return EBTNodeResult::Failed;
	}

	UWorld* World = ControlledPawn->GetWorld();
	if (!World)
	{
		return EBTNodeResult::Failed;
	}

	ABaseCharacter* ClosestPlayer = nullptr;
	float ClosestDistanceSq = TNumericLimits<float>::Max();
	const FVector AlienLocation = ControlledPawn->GetActorLocation();

	for (TActorIterator<ABaseCharacter> It(World); It; ++It)
	{
		ABaseCharacter* Candidate = *It;
		if (!IsValid(Candidate))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(AlienLocation, Candidate->GetActorLocation());
		if (DistanceSq < ClosestDistanceSq)
		{
			ClosestDistanceSq = DistanceSq;
			ClosestPlayer = Candidate;
		}
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	BlackboardComp->SetValueAsObject(BlackboardKey.SelectedKeyName, ClosestPlayer);
	return ClosestPlayer ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}
