// Fill out your copyright notice in the Description page of Project Settings.


#include "RewindFunctionLibrary.h"
#include "RewindManager.h"
#include "Kismet/GameplayStatics.h"
#include "RewindTraceTypes.h"

int32 URewindFunctionLibrary::StaticGetTimeStamp( UObject* WorldContextObject )
{
	return ARewindManager::StaticGetTimeStamp( WorldContextObject );
}

bool URewindFunctionLibrary::ServerRewindLineTrace( UObject* WorldContextObject, int32 TimeStamp, const FVector& TraceStart, const FVector& TraceEnd, TArray<FHitResult>& HitResults )
{
	UWorld* World = GEngine->GetWorldFromContextObject( WorldContextObject, EGetWorldErrorMode::LogAndReturnNull );
	if( !World ) return false;

	ARewindManager* pRewindMgr = Cast< ARewindManager >( UGameplayStatics::GetActorOfClass( WorldContextObject, ARewindManager::StaticClass() ) );
	if( pRewindMgr )
	{
		bool bBlockingHit;
		if( pRewindMgr->LineTrace( (uint32)TimeStamp, TraceStart, TraceEnd, bBlockingHit, HitResults ) )
		{
			return bBlockingHit;
		}
	}

	FCollisionQueryParams Params = FCollisionQueryParams::DefaultQueryParam;
	Params.bReturnPhysicalMaterial = true;

	bool bBlockingHit = World->LineTraceMultiByChannel( HitResults, TraceStart, TraceEnd, Trace_Hitscan, Params, FCollisionResponseParams::DefaultResponseParam );
	return bBlockingHit;
}