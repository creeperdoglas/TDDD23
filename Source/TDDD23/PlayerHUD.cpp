// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerHUD.h"
#include "TDDD23/InputPlayer/InputCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Internationalization/Text.h"


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

bool UPlayerHUD::IsCountdownActive() const
{
    return Player ? Player->IsCountdownActive() : false;
}

float UPlayerHUD::GetCountdownPercent() const
{
    if (Player && Player->IsCountdownActive())
    {
        const float total = Player->DefaultCountdownDuration; //  could also be cached maybe?
        return (total > 0.f) ? (Player->GetCountdownRemaining() / total) : 0.f;
    }
    return 0.f;
}

FText UPlayerHUD::GetCountdownText() const
{
    int32 secs = 0;
    if (Player && Player->IsCountdownActive())
    {
        secs = FMath::Max(0, FMath::RoundToInt(Player->GetCountdownRemaining()));
    }
    const int32 m = secs / 60;
    const int32 s = secs % 60;
    return FText::FromString(FString::Printf(TEXT("%02d:%02d"), m, s));
}