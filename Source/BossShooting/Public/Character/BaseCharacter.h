#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseCharacter.generated.h"

class ABaseWeapon; // Forward declaration 추가

// Forward declarations — 헤더 의존성 최소화
class USpringArmComponent;
class UCameraComponent;
// ★ Forward declarations 추가
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
UCLASS()
class BOSSSHOOTING_API ABaseCharacter : public ACharacter
{
	GENERATED_BODY()


	
public:
	ABaseCharacter();
	
	// 시작 시 자동 장착할 무기 클래스 (BP에서 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<ABaseWeapon> DefaultWeaponClass;
	
	// 현재 장착 중인 무기 - Replicated 포인터
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon, VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<ABaseWeapon> CurrentWeapon;
	
	UFUNCTION()
	void OnRep_CurrentWeapon(ABaseWeapon* OldWeapon);
	
	// 무기 장착 (서버에서만 호출)
	void EquipWeapon(ABaseWeapon* NewWeapon);
	
protected:
	virtual void BeginPlay() override;

	// 탑뷰 카메라 — SpringArm으로 거리/각도 고정
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;
	// ★ Health — 서버 권위, 변경 시 OnRep_Health 콜백
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;

	// ★ MaxHealth — 디자인 값이라 리플리케이션 불필요 (모든 인스턴스 동일 시작)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float MaxHealth;

	// ★ RepNotify 콜백 — 클라에서 값이 도착하면 자동 호출됨
	UFUNCTION()
	void OnRep_Health(float OldHealth);
	
	// Input - BP에서 자산 할당받음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;
	
	void Move(const FInputActionValue& Value);
	
	// ★ 매 틱 마우스 추적 회전
	void UpdateAimRotation();

	// ★ Server RPC — 클라가 서버에 회전 알림 → 서버가 다른 클라에 자동 동기화
	UFUNCTION(Server, Unreliable)
	void Server_SetAimRotation(FRotator NewRotation);
	
	// 발사 Input Action
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category ="Input")
	TObjectPtr<UInputAction> FireAction;
	
	// 발사 핸들러
	void OnFirePressed();
	
public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// ★ 리플리케이션 등록 — 어떤 변수가 네트워크로 동기화될지 선언
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ★ 데미지 처리 — Engine 가상 함수 오버라이드 (서버 권위)
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category = "Character|State")
	bool IsDead() const { return CurrentHealth <= 0.0f; }

	// ★ 디버그 — 콘솔에서 자기 자신에게 데미지
	UFUNCTION(Exec, BlueprintCallable, Category = "Debug")
	void DebugSelfDamage(float Amount);
};
