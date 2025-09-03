// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHUD.generated.h"

/**
 *
 */
UCLASS()
class TDDD23_API UPlayerHUD : public UUserWidget
{
	GENERATED_BODY()
};

protected:
virtual void NativeConstruct() override;

/** Function to calculate Player HP Percentage */
UFUNCTION(BlueprintPure)
float CalculateHealthPercentage();

/** The player */
UPROPERTY(VisibleAnywhere)
ACustomCharacter *Player;