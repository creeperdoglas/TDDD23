// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerHUD.h"
#include "TDDD23Character.h"
#include "Kismet/GameplayStatics.h"


void UPlayerHUD::NativeConstruct()
{
  Super::NativeConstruct();
  Player = Cast<ATDDD23Character>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
}

float UPlayerHUD::CalculateHealthPercentage()
{
  if (Player)
  {
    return Player->GetHealth() / Player->GetMaxHealth();
  }
  return 0.f;
}