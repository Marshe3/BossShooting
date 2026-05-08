#include "Character/BaseCharacter.h"
#include "Engine/DamageEvents.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"  // ★ 추가: DOREPLIFETIME 매크로

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
	SpringArm->TargetArmLength = 1000.0f;                          // 위에서 1200 단위 거리
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
	
	// ★ Stats 초기값
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 이 클래스의 리플리케이트 변수를 등록
	DOREPLIFETIME(ABaseCharacter, CurrentHealth);
}

float ABaseCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 안전장치: 서버에서만 처리 (클라에서 호출되면 무시)
	if (!HasAuthority())
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage > 0.0f)
	{
		const float OldHealth = CurrentHealth;
		CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);

		// ⚠️ 핵심: 서버에서는 RepNotify가 자동 호출 안 됨 → 수동 호출
		OnRep_Health(OldHealth);

		UE_LOG(LogTemp, Warning, TEXT("[서버] %s: %.1f -> %.1f (데미지 %.1f)"),
			*GetName(), OldHealth, CurrentHealth, ActualDamage);
	}

	return ActualDamage;
}

void ABaseCharacter::OnRep_Health(float OldHealth)
{
	// 서버: TakeDamage에서 수동 호출
	// 클라: 서버에서 값이 도착하면 엔진이 자동 호출
    
	const FString RoleStr = HasAuthority() ? TEXT("서버") : TEXT("클라");
	UE_LOG(LogTemp, Warning, TEXT("[%s] %s OnRep_Health: %.1f -> %.1f"),
		*RoleStr, *GetName(), OldHealth, CurrentHealth);

	// 추후 : 향후 UI 갱신, 사망 처리, 피격 이펙트 위치
}

void ABaseCharacter::DebugSelfDamage(float Amount)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("DebugSelfDamage: 서버(호스트) 콘솔에서만 실행 가능"));
		return;
	}

	TakeDamage(Amount, FDamageEvent(), GetController(), this);
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