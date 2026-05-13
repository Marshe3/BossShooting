#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindClosestPlayer.generated.h"

UCLASS()
class BOSSSHOOTING_API UBTTask_FindClosestPlayer : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_FindClosestPlayer();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
