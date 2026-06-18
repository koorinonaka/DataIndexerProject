// Copyright 2026 koorinonaka, All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Styling/SlateColor.h"

#include "DataIndexerMVVMHelper.generated.h"

UCLASS()
class UDataIndexerMVVMHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION( BlueprintPure )
	static FLinearColor LinearColorWithAlpha( FLinearColor Color, float Alpha ) { return Color.CopyWithNewOpacity( Alpha ); }

	UFUNCTION( BlueprintPure )
	static FSlateColor SlateColorWithAlpha( FLinearColor Color, float Alpha ) { return Color.CopyWithNewOpacity( Alpha ); }
};
