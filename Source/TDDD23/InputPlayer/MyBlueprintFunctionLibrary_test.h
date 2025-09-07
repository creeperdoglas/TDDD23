// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyBlueprintFunctionLibrary_test.generated.h"

/**
 * 
 */
UCLASS()
class TDDD23_API UMyBlueprintFunctionLibrary_test : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
protected:

	//Testing if function appears in blueprint
	UFUNCTION(BlueprintCallable, Category = "Input")
	static void DoTestInput();
};
