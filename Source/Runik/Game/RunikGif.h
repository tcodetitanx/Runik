// Animated GIF export of a running machine: a CPU-side board renderer feeding
// the engine-agnostic GIF89a encoder in Core/RunikGifEnc.h.
#pragma once

#include "CoreMinimal.h"
#include "../Core/RunikSim.h"
#include "../Core/RunikGifEnc.h"

class FRunikAssets;

class FRunikBoardRenderer
{
public:
	explicit FRunikBoardRenderer(const FRunikAssets& InAssets) : Assets(InAssets) {}

	// Renders both planes side by side if the machine uses the ethereal plane.
	TArray<FColor> Render(const rk::Sim& Sim, int32& OutW, int32& OutH) const;

	// Runs the machine from scratch and encodes one frame per tick.
	bool ExportGif(const rk::Puzzle& Puzzle,
	               const std::vector<rk::Placement>& Solution,
	               int32 MaxTicks, const FString& FilePath) const;

private:
	const FRunikAssets& Assets;

	void Blit(TArray<FColor>& Dst, int32 DstW, int32 DstH, FName Sprite,
	          float CX, float CY, float Size, FLinearColor Tint) const;
	void FillHex(TArray<FColor>& Dst, int32 DstW, int32 DstH,
	             float CX, float CY, float Size, FColor Color) const;
};
