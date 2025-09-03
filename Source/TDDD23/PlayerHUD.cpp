// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerHUD.h"

void UPlayerHUDWidget::NativeConstruct()
{
  Super::NativeConstruct();
  Player = Cast<ACustomCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
}

float UPlayerHUDWidget::CalculateHealthPercentage()
{
  if (Player)
  {
    return Player->GetHealth() / Player->GetMaxHealth();
  }
  return 0.f;
}