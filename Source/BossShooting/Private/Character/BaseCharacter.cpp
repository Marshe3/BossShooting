#include "Character/BaseCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// === 멀티플레이 기본 ===
	// 서버에서 만든 이 액터를 클라들에 복제
	bReplicates = true;
	// 위치/회전 자동 동기화 (Character는 기본 true지만 명시)
	SetReplicateMovement(true);

	// === 카메라 셋업 (탑뷰) ===
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 1200.0f;                          // 위에서 1200 단위 거리
	SpringArm->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));  // 60도 내려다봄
	SpringArm->bDoCollisionTest = false;                            // 탑뷰는 카메라 충돌 의미 없음
	SpringArm->bUsePawnControlRotation = false;                     // 컨트롤러 회전 무시 (탑뷰 고정)
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	// === 캐릭터 회전 ===
	// 컨트롤러 회전이 캐릭터 메쉬에 영향 X — 마우스 방향으로 직접 돌릴 거니까
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// CMC가 이동 방향으로 자동 회전 X (마우스 방향으로 돌릴 거)
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}