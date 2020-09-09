#pragma once
#include "GameFramework/Character.h"
#include "RewindTraceCharacter.generated.h"

UCLASS(BlueprintType,Blueprintable)
class ARewindTraceCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	UFUNCTION( BlueprintCallable )
	void Shoot( const FVector& TraceStart, const FVector& TraceEnd );

protected:
	UFUNCTION( Server, Reliable )
	void ServerShoot( int32 TimeStamp, const FVector& TraceStart, const FVector& TraceEnd );
};