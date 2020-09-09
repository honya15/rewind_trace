#include "RewindManager.h"
#include "RewindTrace.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "RewindTraceTypes.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "Components/ArrowComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

// maximum amount of lag we can rewind to
float MaxLag = 0.25f;
// interval to make snapshots
float SnapshotInterval = 0.01666f;

const uint32 MaxSnapshotCount = FMath::TruncToInt( MaxLag / SnapshotInterval );

FName RewindCollisionObjectProfile = TEXT( "RewindObject" );
FName NAME_RewindComponent = TEXT( "Rewind" );

ARewindTempActor::ARewindTempActor()
{
	USceneComponent* TempRoot = CreateDefaultSubobject<USceneComponent>( TEXT("RootComponent") );
	RootComponent = TempRoot;

#if WITH_EDITORONLY_DATA
	UArrowComponent* ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>( TEXT( "Arrow" ) );
	if( ArrowComponent )
	{
		ArrowComponent->ArrowColor = FColor( 150, 200, 255 );
		ArrowComponent->bTreatAsASprite = true;
		ArrowComponent->SpriteInfo.Category = TEXT( "Characters" );
		ArrowComponent->SpriteInfo.DisplayName = NSLOCTEXT( "SpriteCategory", "Characters", "Characters" );
		ArrowComponent->SetupAttachment( TempRoot );
		ArrowComponent->bIsScreenSizeScaled = true;
		ArrowComponent->SetVisibility( true );
		ArrowComponent->SetHiddenInGame( false );
	}
#endif // WITH_EDITORONLY_DATA
}

void ARewindTempActor::Init( const FRewindSnapshotActor& SnapshotActor )
{
	for( const FRewindSnapshotCollision& Collision : SnapshotActor.CollisionShapes )
	{
		UPrimitiveComponent* Component = nullptr;

		switch( Collision.ShapeType )
		{
		case ECollisionShape::Box:
			{
				UBoxComponent* BoxComp = NewObject< UBoxComponent >( this, Collision.ComponentName );
				BoxComp->InitBoxExtent( Collision.Dimensions );
				Component = BoxComp;
			}
			break;
		case ECollisionShape::Capsule:
			{
				UCapsuleComponent* CapsuleComp = NewObject< UCapsuleComponent >( this, Collision.ComponentName );
				CapsuleComp->InitCapsuleSize( Collision.Dimensions.X, Collision.Dimensions.Z );
				Component = CapsuleComp;
			}
			break;
		case ECollisionShape::Sphere:
			{
				USphereComponent* SphereComp = NewObject< USphereComponent >( this, Collision.ComponentName );
				SphereComp->InitSphereRadius( Collision.Dimensions.X );
				Component = SphereComp;
			}
			break;
		}

		if( !Component ) continue;
		Component->AttachToComponent( GetRootComponent(), FAttachmentTransformRules( EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, true ) );
		Component->SetWorldTransform( Collision.Transform );
		Component->SetCollisionProfileName( RewindCollisionObjectProfile );
		Component->SetVisibility( true );
		Component->SetHiddenInGame( false );
		Component->RegisterComponent();
		CollisionComponents.Add( Component );
	}
}

ARewindManager::ARewindManager()
	: SnapshotQueue( MaxSnapshotCount )
{

}

ARewindManager::ARewindManager( FVTableHelper& Helper )
	: Super( Helper )
	, SnapshotQueue( MaxSnapshotCount )
{

}

void ARewindManager::BeginPlay()
{
	Super::BeginPlay();

	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer( Handle, this, &ARewindManager::TickSnapshot, SnapshotInterval, true );
}

void ARewindManager::TickSnapshot()
{
	MakeSnapshot( StaticGetTimeStamp( this ) );
}

void ARewindManager::MakeSnapshot( uint32 TimeStamp )
{
	FRewindSnapshot& Snapshot = SnapshotQueue[ TimeStamp ];
	if( Snapshot.TimeStamp == TimeStamp )
	{
		// already done this timestamp (unreal can fire off for looping event multiple times)
		return;
	}

	Snapshot = FRewindSnapshot();
	Snapshot.TimeStamp = TimeStamp;
	
	TArray< AActor* > Actors;
	UGameplayStatics::GetAllActorsOfClass( this, ACharacter::StaticClass(), Actors );

	for( AActor* pActor : Actors )
	{
		TArray< UActorComponent* > Components = pActor->GetComponentsByTag( UPrimitiveComponent::StaticClass(), NAME_RewindComponent );
		if( Components.Num() )
		{
			FRewindSnapshotActor& SnapshotActor = Snapshot.Actors.AddDefaulted_GetRef();
			SnapshotActor.Actor = pActor;
			SnapshotActor.Transform = pActor->GetActorTransform();
			
			for( UActorComponent* Component : Components )
			{
				UPrimitiveComponent* PrimComp = Cast< UPrimitiveComponent >( Component );
				if( !ensure( PrimComp ) ) continue;
				
				FCollisionShape Shape = PrimComp->GetCollisionShape();
				SnapshotActor.CollisionShapes.Emplace( PrimComp->GetFName(), PrimComp->GetComponentTransform(), Shape.GetExtent(), Shape.ShapeType );
			}
		}
	}
}

bool ARewindManager::LineTrace( uint32 TimeStamp, const FVector& TraceStart, const FVector& TraceEnd, bool& bBlockingHit, TArray<FHitResult>& HitResults )
{
	const FRewindSnapshot& Snapshot = SnapshotQueue[ TimeStamp ];
	if( Snapshot.TimeStamp != TimeStamp )
	{
		// not stored snapshot for this timestamp
		return false;
	}

	UWorld* pWorld = GetWorld();

	FActorSpawnParameters SpawnParams;
	SpawnParams.bDeferConstruction = true;

	TArray< AActor* > SpawnedActors;

	for( const FRewindSnapshotActor& SnapshotActor : Snapshot.Actors )
	{
		if( SnapshotActor.Actor )
		{
			ARewindTempActor* SpawnedActor = Cast< ARewindTempActor >( pWorld->SpawnActor( ARewindTempActor::StaticClass(), &SnapshotActor.Transform, SpawnParams ) );
			if( SpawnedActor )
			{
				SpawnedActor->Init( SnapshotActor );
				UGameplayStatics::FinishSpawningActor( SpawnedActor, SnapshotActor.Transform );
			}
			
			SpawnedActors.Add( SpawnedActor );
		}
	}

	FCollisionQueryParams Params = FCollisionQueryParams::DefaultQueryParam;
	Params.bReturnPhysicalMaterial = true;

	bBlockingHit = pWorld->LineTraceMultiByChannel( HitResults, TraceStart, TraceEnd, Trace_Rewind, Params, FCollisionResponseParams::DefaultResponseParam );

	for( FHitResult& Result : HitResults )
	{
		// look back for the original component
		if( AActor* HitActor =  Result.GetActor() )
		{
			// we use the fact that the indexing of SpawnedActors array is same as the snapshot actors
			int32 nIndex = SpawnedActors.IndexOfByKey( HitActor );
			if( nIndex != INDEX_NONE )
			{
				const FRewindSnapshotActor& SnapshotActor = Snapshot.Actors[ nIndex ];

				// modify the result actor
				Result.Actor = SnapshotActor.Actor;

				if( UPrimitiveComponent* Component = Result.GetComponent() )
				{
					// find the original component, by name
					for( UActorComponent* ActorComp : Result.Actor->GetComponents() )
					{
						if( ActorComp->GetFName() == Component->GetFName() )
						{
							// modify the result component
							Result.Component = Cast< UPrimitiveComponent >( ActorComp );
							Result.PhysMaterial = Result.Component->GetBodyInstance()->GetSimplePhysicalMaterial();
							break;
						}
					}
				}
			}
		}
	}

	for( AActor* SpawnedActor : SpawnedActors )
	{
		if( SpawnedActor )
		{
			SpawnedActor->Destroy();
			// SpawnedActor->SetLifeSpan( 1.0f );
		}
	}

	return true;
}

int32 ARewindManager::StaticGetTimeStamp( UObject* WorldContextObject )
{
	UWorld* World = GEngine->GetWorldFromContextObject( WorldContextObject, EGetWorldErrorMode::LogAndReturnNull );
	if( !World ) return 0;

	float fTimeSeconds;

	if( !World->GetGameState() )
	{
		fTimeSeconds = World->GetTimeSeconds();
	}
	else
	{
		fTimeSeconds = World->GetGameState()->GetServerWorldTimeSeconds();
	}

	return FMath::TruncToInt( fTimeSeconds / SnapshotInterval );
}
