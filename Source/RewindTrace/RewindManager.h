#pragma once
#include "GameFramework/Actor.h"

#include "RewindManager.generated.h"

USTRUCT()
struct FRewindSnapshotCollision
{
	GENERATED_BODY()

	FRewindSnapshotCollision() = default;
	FRewindSnapshotCollision( const FName& _Name, const FTransform& _Transform, const FVector& _Dimensions, ECollisionShape::Type _Type )
		: ComponentName( _Name )
		, Transform( _Transform )
		, Dimensions( _Dimensions )
		, ShapeType( _Type ) {}

	FName ComponentName;
	FTransform Transform;
	FVector Dimensions;
	ECollisionShape::Type ShapeType;
};

USTRUCT()
struct FRewindSnapshotActor
{
	GENERATED_BODY()

	UPROPERTY()
	AActor* Actor;
	FTransform Transform;
	TArray< FRewindSnapshotCollision > CollisionShapes;
};


USTRUCT()
struct FRewindSnapshot
{
	GENERATED_BODY()
	uint32 TimeStamp = UINT_MAX;
	TArray< FRewindSnapshotActor > Actors;
};

UCLASS()
class ARewindTempActor : public AActor
{
	GENERATED_BODY()
public:
	ARewindTempActor();
	void Init( const FRewindSnapshotActor& SnapshotActor );

protected:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray< UPrimitiveComponent* > CollisionComponents;
};

UCLASS(Blueprintable)
class ARewindManager : public AActor
{
	GENERATED_BODY()
public:
	ARewindManager();
	ARewindManager( FVTableHelper& Helper );

	void BeginPlay() override;

	UFUNCTION()
	void TickSnapshot();

	void MakeSnapshot( uint32 TimeStamp );
	bool LineTrace( uint32 TimeStamp, const FVector& TraceStart, const FVector& TraceEnd, bool& bBlockingHit, TArray< FHitResult >& HitResults );

	static int32 StaticGetTimeStamp( UObject* WorldContextObject );
private:
	TCircularBuffer< FRewindSnapshot > SnapshotQueue;
};