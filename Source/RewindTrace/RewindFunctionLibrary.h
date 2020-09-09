// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RewindFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class REWINDTRACE_API URewindFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION( BlueprintCallable, meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject") )
		static int32 StaticGetTimeStamp( UObject* WorldContextObject );

	UFUNCTION( BlueprintCallable, meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject") )
		static bool ServerRewindLineTrace( UObject* WorldContextObject, int32 TimeStamp, const FVector& TraceStart, const FVector& TraceEnd, TArray< FHitResult >& HitResults );
};
