#include "Character/BaseCharacter.h"
#include "Engine/DamageEvents.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"  // ★ 추가: DOREPLIFETIME 매크로
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

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
	
	// ★ 로컬 플레이어의 입력 시스템에 IMC 등록
	// 이 캐릭터를 조종하는 PlayerController가 있을 때만 실행됨
	// (서버가 시뮬레이션하는 다른 클라의 캐릭터는 PlayerController 없음)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// ★ 마우스 커서 표시 + 입력 모드
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
		
		// IMC 등록 코드
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = 
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext,  0);
			}
		}
	}
	
	
	
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// ★ 로컬에서 조종하는 캐릭터만 마우스 추적
	// - 호스트 입장에서 자기 캐릭터: IsLocallyControlled = true → 추적
	// - 호스트 입장에서 다른 클라 캐릭터: IsLocallyControlled = false → 추적 X (그 클라가 알아서 함)
	if (IsLocallyControlled())
	{
		UpdateAimRotation();
	}
	
}

void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// PlayerInputComponent는 기본 UInputComponent라 Enhanced로 캐스팅 필요
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABaseCharacter::Move);
		}
	}
	
}

void ABaseCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MoveInput = Value.Get<FVector2D>();
	
	if (Controller && !MoveInput.IsZero())
	{
		// 월드 좌표 기준 이동 (탑뷰라 카메라 회전 신경 안 써도 됨 — 카메라 고정)
		AddMovementInput(FVector::ForwardVector, MoveInput.Y);
		AddMovementInput(FVector::RightVector, MoveInput.X);
	}
	 
}

void ABaseCharacter::UpdateAimRotation()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;
	
	FHitResult Hit;
	PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit);
	
	if (!Hit.bBlockingHit) return;
	
	FVector ToTarget = Hit.ImpactPoint - GetActorLocation();
	ToTarget.Z = 0.0f; // 수평 평면 회전만 (탑뷰)
	
	if (ToTarget.IsNearlyZero()) return;
	
	const FRotator NewRotation(0.0f, ToTarget.Rotation().Yaw, 0.0f);
	
	// 1) 로컬에서 즉시 회전 (반응성 — 자기 화면에서는 지연 없이)
	SetActorRotation(NewRotation);
	
	// 2) 권위 없으면(= 클라이언트면) 서버에 알림
	//    서버는 받아서 자기 캐릭터를 회전시키고, bReplicateMovement가 그걸 다른 클라에 자동 동기화
	if (!HasAuthority())
	{
		Server_SetAimRotation(NewRotation);
	}
}

void ABaseCharacter::Server_SetAimRotation_Implementation(FRotator NewRotation)
{
	// 서버 권위 회전 적용
	// → Character의 bReplicateMovement = true 덕분에
	//   서버 → 다른 클라들에 위치/회전 자동 전파
	SetActorRotation(NewRotation);
	
}