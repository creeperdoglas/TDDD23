// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerHUD.h"
#include "TDDD23/InputPlayer/InputCharacter.h"
#include "Kismet/GameplayStatics.h"


void UPlayerHUD::NativeConstruct()
{
  Super::NativeConstruct();
  Player = Cast<AInputCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
}

float UPlayerHUD::CalculateHealthPercentage()
{
	if (Player)
	{
		const float Max = Player->GetMaxHealth();
		return Max > 0.f ? Player->GetHealth() / Max : 0.f;
	}
	return 0.f;
}