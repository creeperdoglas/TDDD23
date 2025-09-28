#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyNPC.generated.h"

class USphereComponent;
class AInputCharacter;
class AAIController;

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

	
	//sekunder mellan attacker när spelare är i attack range (AttackSphere)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.1"))
	float AttackInterval = 1.25f;


	//delay för första hit, gör det lätt att synca animation om jag förstår rätt
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0"))
	float FirstHitDelay = 0.15f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bInAttackRange = false;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	AInputCharacter* TargetPlayer = nullptr;

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

	// === Movement ===
	void StartChasingTarget();
	void StopChasingTarget();

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
	UFUNCTION(BlueprintImplementableEvent, Category = "Damage") void BP_OnDamaged(float Damage);
	UFUNCTION(BlueprintImplementableEvent, Category = "Damage") void BP_OnDeath();
	UFUNCTION(BlueprintImplementableEvent, Category = "AI")     void BP_OnPlayerSpotted(AActor* NewTarget);
	UFUNCTION(BlueprintImplementableEvent, Category = "AI")     void BP_OnPlayerLost();
};