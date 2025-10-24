// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "InputCharacter.generated.h"


UCLASS()
class TDDD23_API AInputCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AInputCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 100.f;

	UFUNCTION(BlueprintCallable, Category = "Stats")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintCallable, Category = "Stats")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Damage")
	void BP_OnDeath();

	UFUNCTION(BlueprintImplementableEvent, Category = "Stats")
	void BP_OnHealthChanged();

	virtual float TakeDamage(float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;


	UFUNCTION(BlueprintImplementableEvent, Category = "Damage")
	void BP_OnDamaged(float Damage);

	// --- Countdown timer (game-end challenge) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer")
	float DefaultCountdownDuration = 30.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timer")
	bool bCountdownActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timer")
	float CountdownRemaining = 0.f;

	FTimerHandle CountdownTimerHandle;

	// Control
	UFUNCTION(BlueprintCallable, Category = "Timer")
	void StartCountdown(float Duration /*use -1 to use DefaultCountdownDuration*/ = -1.f);

	UFUNCTION(BlueprintCallable, Category = "Timer")
	void CancelCountdown();           // abort without killing player

	UFUNCTION(BlueprintCallable, Category = "Timer")
	void SucceedCountdown();          // call when the player reaches the goal in time

	// Read
	UFUNCTION(BlueprintPure, Category = "Timer")
	bool IsCountdownActive() const { return bCountdownActive; }

	UFUNCTION(BlueprintPure, Category = "Timer")
	float GetCountdownRemaining() const { return CountdownRemaining; }

	// Internals
	UFUNCTION() void CountdownTick();

	// Optional BP hooks for UI/SFX
	UFUNCTION(BlueprintImplementableEvent, Category = "Timer")
	void BP_OnCountdownTick(float SecondsRemaining);

	UFUNCTION(BlueprintImplementableEvent, Category = "Timer")
	void BP_OnCountdownFinished(bool bSuccess);

protected:

	UPROPERTY(EditAnywhere, Category = "EnhancedInput")
	class UInputMappingContext* InputMapping;
	
	UPROPERTY(EditAnywhere, Category = "EnhancedInput")
	class UInputAction* TestAction;

	void TestInput();
};
