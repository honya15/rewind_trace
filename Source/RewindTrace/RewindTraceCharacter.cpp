#include "RewindTraceCharacter.h"
#include "RewindTrace.h"
#include "Engine/Engine.h"
#include "RewindFunctionLibrary.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "RewindTraceTypes.h"


void ARewindTraceCharacter::Shoot( const FVector& TraceStart, const FVector& TraceEnd )
{
	ServerShoot( URewindFunctionLibrary::StaticGetTimeStamp( this ), TraceStart, TraceEnd );
}

void ARewindTraceCharacter::ServerShoot_Implementation( int32 TimeStamp, const FVector& TraceStart, const FVector& TraceEnd )
{
	TArray< FHitResult > HitResults;
	bool bBlockingHit = URewindFunctionLibrary::ServerRewindLineTrace( this, TimeStamp, TraceStart, TraceEnd, HitResults );

	const FColor DebugColor = FColor::Red;

	for( const FHitResult& HitRes : HitResults )
	{
		if( HitRes.PhysMaterial.IsValid() )
		{
			if( HitRes.PhysMaterial->SurfaceType == SurfaceType_Body )
			{
				GEngine->AddOnScreenDebugMessage( INDEX_NONE, 1.0f, DebugColor, TEXT( "Body shot" ) );
				break;
			}
			else if( HitRes.PhysMaterial->SurfaceType == SurfaceType_Head )
			{
				GEngine->AddOnScreenDebugMessage( INDEX_NONE, 1.0f, DebugColor, TEXT( "Head shot" ) );
				break;
			}
		}
	}
}
