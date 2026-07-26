// Runtime loading of the raw art in Content/Raw into both CPU pixel buffers
// (for the GIF renderer) and GPU textures wrapped in Slate brushes (for UI).
#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"

class UTexture2D;

struct FRunikSprite
{
	int32 Width = 0;
	int32 Height = 0;
	TArray<FColor> Pixels;      // BGRA as FColor
	UTexture2D* Texture = nullptr;
	FSlateBrush Brush;
};

class FRunikAssets
{
public:
	// Loads every sprite; textures are added to KeepAlive for GC rooting.
	void LoadAll(TArray<TObjectPtr<UTexture2D>>& KeepAlive);

	const FRunikSprite* Get(FName Name) const;
	const FSlateBrush* GetBrush(FName Name) const;

	static FName GemSpriteName(int32 GemIndex);
	static FName RuneSpriteName(int32 RuneIndex);

private:
	TMap<FName, FRunikSprite> Sprites;
	bool LoadSprite(FName Name, const FString& Path,
	                TArray<TObjectPtr<UTexture2D>>& KeepAlive);
};
