// Fill out your copyright notice in the Description page of Project Settings.


#include "InputPlayer/MyBlueprintFunctionLibrary_test.h"

void UMyBlueprintFunctionLibrary_test::DoTestInput()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, "Pressed input action");
}