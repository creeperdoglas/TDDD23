// Fill out your copyright notice in the Description page of Project Settings.


#include "TDDD23/InputPlayer/InputCharacter.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "TimerManager.h" 

// Sets default values
AInputCharacter::AInputCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AInputCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AInputCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AInputCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//add input mapping context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		//Get local player subsystem
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			//Add input context
			Subsystem->AddMappingContext(InputMapping, 0);
		}
	}

	if (UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(TestAction, ETriggerEvent::Triggered, this, &AInputCharacter::TestInput);
	}
}

void AInputCharacter::TestInput()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, "Pressed input action");
}

float AInputCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	const float Actual = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	Health = FMath::Clamp(Health - Actual, 0.f, MaxHealth);
	BP_OnDamaged(Actual); // Let Blueprint/UI react (screen flash, sounds, etc.)
	BP_OnHealthChanged();
	if (Health <= 0.f)
	{
		BP_OnDeath(); 
	}
	return Actual;
}

//new
void AInputCharacter::StartCountdown(float Duration)
{
    if (bCountdownActive)
    {
        CancelCountdown();
    }

    const float UseDuration = (Duration > 0.f) ? Duration : DefaultCountdownDuration;
    CountdownRemaining = UseDuration;
    bCountdownActive = true;

    // tick every 0.1s for smooth UI
    GetWorldTimerManager().SetTimer(CountdownTimerHandle, this, &AInputCharacter::CountdownTick, 0.1f, true);

    // initial notify
    BP_OnCountdownTick(CountdownRemaining);
}

void AInputCharacter::CancelCountdown()
{
    GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
    bCountdownActive = false;
}

void AInputCharacter::SucceedCountdown()
{
    if (!bCountdownActive) return;
    GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
    bCountdownActive = false;
    BP_OnCountdownFinished(true);
}

void AInputCharacter::CountdownTick()
{
    if (!bCountdownActive) return;

    CountdownRemaining = FMath::Max(0.f, CountdownRemaining - 0.1f);
    BP_OnCountdownTick(CountdownRemaining);

    if (CountdownRemaining <= 0.f)
    {
        // Time’s up → kill player using our existing flow
        GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
        bCountdownActive = false;

        Health = 0.f;                   // update stats
        BP_OnHealthChanged();           // refresh HUD etc
        BP_OnDeath();                   // let BP handle ragdoll/respawn, as  in TakeDamage branch 
        BP_OnCountdownFinished(false);  // optional UI/SFX hook
    }
}
