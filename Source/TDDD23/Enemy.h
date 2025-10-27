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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	float MeshYawOffset = -90.f;

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


	// --- Locomotion assets (is assigned in the enemy BP instance) ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion")
	UAnimSequence* Idle_Sequence = nullptr;      // asset: "Idle"

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion")
	UAnimSequence* Jog_Fwd_Sequence = nullptr;   // asset: "Jog_Fwd"   (or "Jog_Fwd_Start/Stop" if we later want starts/stops)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Locomotion")
	UAnimSequence* Run_Fwd_Sequence = nullptr;   // asset: "Run_Fwd"

	// Use same slot as your attacks unless you prefer a different one in your AnimBP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Locomotion")
	FName LocomotionSlotName = TEXT("DefaultSlot");

	// Speed thresholds (tune in BP)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Locomotion")
	float WalkSpeedThreshold = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Locomotion")
	float RunSpeedThreshold = 350.f;

	// Runtime bookkeeping
	UAnimMontage* CurrentLocomotionMontage = nullptr;
	UAnimSequence* CurrentLocomotionSeq = nullptr;

	// Helpers
	void EnsureLocomotion(UAnimSequence* Seq);
	void StopLocomotion();
	bool IsAnyAttackMontagePlaying() const;
	// --- Facing settings (pure C++) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Facing")
	float FaceInterpSpeed = 8.f;          // how fast we rotate toward the target

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Facing")
	bool bSnapYawOnAttack = true;         // snap to target at the very start of an attack

	// --- Locomotion slot resolution ---
	// If LocomotionSlotName is left as "DefaultSlot" and that slot doesn't exist in the current AnimBP,
	// we auto-fallback to the first slot used by the attack montage, else to direct PlayAnimation.
	FName ResolveLocomotionSlot() const;

	// --- Explicit facing helper (called from Tick) ---
	void FaceTarget(float DeltaSeconds);

	// --- Runtime flag: when true we are temporarily using Single-Node mode for locomotion while chasing
	bool bUsingSingleNodeLocomotion = false;

	// Switch mesh animation mode at runtime (Single Node for run/jog, ABP for idle/attacks)
	void EnterSingleNodeLocomotion(UAnimSequence* SeqToLoop);
	void ExitSingleNodeLocomotion();

	// Follow without NavMesh: when false, we steer directly (no pathfinding).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bUseNavMeshForChase = false;

	// Direct-chase step (no NavMesh, due to navmesh casuing issues when building) — called from Tick
	void ManualChaseStep(float DeltaSeconds);

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
