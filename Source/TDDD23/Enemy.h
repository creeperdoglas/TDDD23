// Enemy.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Enemy.generated.h"

class USphereComponent;
class AInputCharacter;
class AAIController;

// ↓ forward declarations so we don’t pull animation headers here
class UAnimMontage;
class UAnimSequence;

UCLASS()
class TDDD23_API AEnemyNPC : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyNPC();
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	// === Components ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	USphereComponent* AggroSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	USphereComponent* AttackSphere;

	// === Stats ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 50.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float Health = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.1"))
	float AttackInterval = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0"))
	float FirstHitDelay = 0.15f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bInAttackRange = false;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	AInputCharacter* TargetPlayer = nullptr;

	// ====== NEW: Animation assets (assign these exact Paragon assets in BP) ======
	// Attacks
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Attack")
	UAnimMontage* PrimaryAttack_RA_Montage = nullptr; // “PrimaryAttack_RA_Montage”
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Attack")
	UAnimMontage* PrimaryAttack_LA_Montage = nullptr; // “PrimaryAttack_LA_Montage”

	// Hit reacts
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Hit")
	UAnimSequence* HitReact_Front = nullptr; // “HitReact_Front”
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Hit")
	UAnimSequence* HitReact_Back = nullptr; // “HitReact_Back”
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Hit")
	UAnimSequence* HitReact_Left = nullptr; // “HitReact_Left”
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Hit")
	UAnimSequence* HitReact_Right = nullptr; // “HitReact_Right”

	// Death
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Death")
	UAnimSequence* Death_Sequence = nullptr; // “Death”

	// Slot used when we wrap sequences as dynamic montages
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	FName MontageSlotName = TEXT("DefaultSlot");

	// ====== NEW: Patrol (optional, light) ======
	UPROPERTY(EditInstanceOnly, Category = "AI|Patrol")
	TArray<AActor*> PatrolPoints;

	UPROPERTY(EditAnywhere, Category = "AI|Patrol")
	float PatrolAcceptanceRadius = 120.f;

	UPROPERTY(EditAnywhere, Category = "AI|Patrol")
	float PatrolSpeed = 220.f;

	UPROPERTY(EditAnywhere, Category = "AI|Chase")
	float ChaseSpeed = 420.f;

	UPROPERTY(EditAnywhere, Category = "Mesh")
	float MeshZOffset = -90.f;   // -90 is a good starting point for Paragon/Mannequin

	// Rotate to face target even while chasing
	UPROPERTY(EditAnywhere, Category = "AI")
	bool bFaceTargetWhileChasing = true;

	int32 CurrentPatrolIndex = 0;

	// === Overlap callbacks ===
	UFUNCTION()
	void OnAggroBegin(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnAggroEnd(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UFUNCTION()
	void OnAttackBegin(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnAttackEnd(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	// === Combat ===
	void StartAttack();
	void StopAttack();
	void PerformAttack();

	// ====== NEW: helper to pick hit-react by direction ======
	void PlayHitReactFrom(const FVector& FromWorldPos);

	// === Movement ===
	void StartChasingTarget();
	void StopChasingTarget();

	// ====== NEW: patrol helpers ======
	void StartPatrol();
	void AdvancePatrolIfClose();

	FTimerHandle AttackTimerHandle;

	// Damage
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;

public:
	// Helpers for Blueprints/UI ifall det behövs
	UFUNCTION(BlueprintPure, Category = "Stats") float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure, Category = "Stats") float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure, Category = "AI")    AInputCharacter* GetTarget() const { return TargetPlayer; }
	UFUNCTION(BlueprintPure, Category = "State") bool IsDead() const { return bIsDead; }

	// Blueprint hooks (animation, VFX, SFX)
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat") void BP_OnAttack();
	UFUNCTION(BlueprintImplementableEvent, Category = "Damage") void BP_OnDamaged(float DamageReceived);
	UFUNCTION(BlueprintImplementableEvent, Category = "Damage") void BP_OnDeath();
	UFUNCTION(BlueprintImplementableEvent, Category = "AI")     void BP_OnPlayerSpotted(AActor* NewTarget);
	UFUNCTION(BlueprintImplementableEvent, Category = "AI")     void BP_OnPlayerLost();
};
