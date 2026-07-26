// Shared UI palette and small helpers for Runik's Slate widgets.
#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/CoreStyle.h"

namespace RunikStyle
{

inline const FLinearColor Gold(1.00f, 0.78f, 0.28f, 1.0f);      // material runes
inline const FLinearColor Azure(0.35f, 0.75f, 1.00f, 1.0f);     // ethereal runes
inline const FLinearColor Parchment(0.88f, 0.83f, 0.72f, 1.0f); // body text
inline const FLinearColor Dim(0.55f, 0.52f, 0.47f, 1.0f);
inline const FLinearColor PanelBg(0.045f, 0.045f, 0.06f, 0.94f);
inline const FLinearColor BoardBg(0.0f, 0.0f, 0.0f, 1.0f);
inline const FLinearColor Danger(0.9f, 0.35f, 0.3f, 1.0f);
inline const FLinearColor Ok(0.55f, 0.85f, 0.45f, 1.0f);

inline FSlateFontInfo Font(int32 Size, bool bBold = false)
{
	return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
}

// Pointy-top hex layout (matches the hex tile and gem medallion art).
// Size is the circumradius: a tile is RootThree*Size wide and 2*Size tall.
constexpr float RootThree = 1.7320508f;

inline FVector2D HexToPixel(int32 Q, int32 R, float Size)
{
	return FVector2D(RootThree * Size * (Q + R * 0.5f), 1.5f * Size * R);
}

inline void PixelToHex(const FVector2D& P, float Size, int32& OutQ, int32& OutR)
{
	const float FR = P.Y / (1.5f * Size);
	const float FQ = P.X / (RootThree * Size) - FR * 0.5f;
	// cube rounding
	const float FS = -FQ - FR;
	float RQ = FMath::RoundToFloat(FQ);
	float RR = FMath::RoundToFloat(FR);
	const float RS = FMath::RoundToFloat(FS);
	const float DQ = FMath::Abs(RQ - FQ);
	const float DR = FMath::Abs(RR - FR);
	const float DS = FMath::Abs(RS - FS);
	if (DQ > DR && DQ > DS)      RQ = -RR - RS;
	else if (DR > DS)            RR = -RQ - RS;
	OutQ = static_cast<int32>(RQ);
	OutR = static_cast<int32>(RR);
}

} // namespace RunikStyle
