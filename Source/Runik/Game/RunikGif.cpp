#include "RunikGif.h"
#include "Runik.h"
#include "RunikAssets.h"
#include "Misc/FileHelper.h"

// ---------------------------------------------------------- board renderer
namespace
{

constexpr float RootThree = 1.7320508f;

void HexToXY(const rk::Hex& H, float Size, float& X, float& Y)
{
	X = 1.5f * Size * H.Q;
	Y = RootThree * Size * (H.R + H.Q * 0.5f);
}

} // namespace

void FRunikBoardRenderer::FillHex(TArray<FColor>& Dst, int32 DstW, int32 DstH,
                                  float CX, float CY, float Size, FColor Color) const
{
	// crude filled hex: horizontal spans of a flat-top hexagon
	const int32 Y0 = FMath::Max(0, static_cast<int32>(CY - Size));
	const int32 Y1 = FMath::Min(DstH - 1, static_cast<int32>(CY + Size));
	for (int32 Y = Y0; Y <= Y1; ++Y)
	{
		const float DY = FMath::Abs(Y - CY) / Size;
		if (DY > RootThree * 0.5f) continue;
		const float HalfW = Size * FMath::Min(1.0f, 1.15f - DY * 0.577f);
		const int32 X0 = FMath::Max(0, static_cast<int32>(CX - HalfW));
		const int32 X1 = FMath::Min(DstW - 1, static_cast<int32>(CX + HalfW));
		for (int32 X = X0; X <= X1; ++X)
		{
			Dst[Y * DstW + X] = Color;
		}
	}
}

void FRunikBoardRenderer::Blit(TArray<FColor>& Dst, int32 DstW, int32 DstH,
                               FName SpriteName, float CX, float CY, float Size,
                               FLinearColor Tint) const
{
	const FRunikSprite* Sprite = Assets.Get(SpriteName);
	if (!Sprite) return;
	const int32 Side = static_cast<int32>(Size * 2.0f);
	const int32 X0 = static_cast<int32>(CX - Size);
	const int32 Y0 = static_cast<int32>(CY - Size);
	for (int32 Y = 0; Y < Side; ++Y)
	{
		const int32 DY = Y0 + Y;
		if (DY < 0 || DY >= DstH) continue;
		const int32 SY = FMath::Clamp(Y * Sprite->Height / Side, 0, Sprite->Height - 1);
		for (int32 X = 0; X < Side; ++X)
		{
			const int32 DX = X0 + X;
			if (DX < 0 || DX >= DstW) continue;
			const int32 SX = FMath::Clamp(X * Sprite->Width / Side, 0, Sprite->Width - 1);
			const FColor Src = Sprite->Pixels[SY * Sprite->Width + SX];
			if (Src.A < 8) continue;
			const float A = (Src.A / 255.0f) * Tint.A;
			FColor& D = Dst[DY * DstW + DX];
			D.R = static_cast<uint8>(FMath::Clamp(Src.R * Tint.R * A + D.R * (1 - A), 0.f, 255.f));
			D.G = static_cast<uint8>(FMath::Clamp(Src.G * Tint.G * A + D.G * (1 - A), 0.f, 255.f));
			D.B = static_cast<uint8>(FMath::Clamp(Src.B * Tint.B * A + D.B * (1 - A), 0.f, 255.f));
		}
	}
}

TArray<FColor> FRunikBoardRenderer::Render(const rk::Sim& Sim,
                                           int32& OutW, int32& OutH) const
{
	const rk::Puzzle& Puzzle = Sim.GetPuzzle();
	const float HexSize = 22.0f;
	const int32 Radius = Puzzle.Radius;
	const int32 PlaneW = static_cast<int32>((Radius * 2 + 2) * 1.5f * HexSize);
	const int32 PlaneH = static_cast<int32>((Radius * 2 + 2) * RootThree * HexSize);

	// Does this machine touch the ethereal plane at all?
	bool bEthereal = false;
	for (const rk::Entity& E : Sim.GetEntities())
	{
		if (E.bAlive && E.Plane == rk::EPlane::Ethereal) { bEthereal = true; break; }
	}
	for (const rk::OutputZone& Z : Puzzle.Outputs)
	{
		if (Z.Plane == rk::EPlane::Ethereal) bEthereal = true;
	}

	OutW = bEthereal ? PlaneW * 2 + 8 : PlaneW;
	OutH = PlaneH;
	TArray<FColor> Pixels;
	Pixels.Init(FColor(16, 16, 20, 255), OutW * OutH);

	const int32 PlaneCount = bEthereal ? 2 : 1;
	for (int32 PlaneIdx = 0; PlaneIdx < PlaneCount; ++PlaneIdx)
	{
		const rk::EPlane Plane = PlaneIdx == 0 ? rk::EPlane::Material
		                                       : rk::EPlane::Ethereal;
		const float OffX = PlaneW * 0.5f + PlaneIdx * (PlaneW + 8);
		const float OffY = PlaneH * 0.5f;

		// board tiles
		for (int32 Q = -Radius; Q <= Radius; ++Q)
		{
			for (int32 R = -Radius; R <= Radius; ++R)
			{
				const rk::Hex H(Q, R);
				if (rk::HexLen(H) > Radius) continue;
				float X, Y;
				HexToXY(H, HexSize, X, Y);
				FillHex(Pixels, OutW, OutH, OffX + X, OffY + Y, HexSize - 1,
				        PlaneIdx == 0 ? FColor(38, 38, 44, 255)
				                      : FColor(30, 36, 50, 255));
			}
		}
		// output zones
		for (const rk::OutputZone& Z : Puzzle.Outputs)
		{
			if (Z.Plane != Plane) continue;
			for (const rk::OutputCell& C : Z.Cells)
			{
				float X, Y;
				HexToXY(C.Pos, HexSize, X, Y);
				FillHex(Pixels, OutW, OutH, OffX + X, OffY + Y, HexSize - 3,
				        FColor(52, 64, 46, 255));
			}
		}
		// spawners
		for (const rk::Spawner& S : Puzzle.Spawners)
		{
			if (S.Plane != Plane) continue;
			for (const rk::SpawnCell& C : S.Cells)
			{
				float X, Y;
				HexToXY(S.Pos + C.Offset, HexSize, X, Y);
				FillHex(Pixels, OutW, OutH, OffX + X, OffY + Y, HexSize - 3,
				        FColor(64, 52, 40, 255));
			}
		}
		// entities
		for (const rk::Entity& E : Sim.GetEntities())
		{
			if (!E.bAlive || E.Plane != Plane) continue;
			float X, Y;
			HexToXY(E.Pos, HexSize, X, Y);
			if (E.Kind == rk::EKind::Gem)
			{
				const FLinearColor Tint = E.bStatic
					? FLinearColor(0.45f, 0.45f, 0.5f, 1.0f) : FLinearColor::White;
				Blit(Pixels, OutW, OutH,
				     FRunikAssets::GemSpriteName(static_cast<int32>(E.Gem)),
				     OffX + X, OffY + Y, HexSize * 0.92f, Tint);
			}
			else
			{
				const FLinearColor Glow = Plane == rk::EPlane::Material
					? FLinearColor(1.0f, 0.78f, 0.28f, 1.0f)
					: FLinearColor(0.35f, 0.75f, 1.0f, 1.0f);
				Blit(Pixels, OutW, OutH,
				     FRunikAssets::RuneSpriteName(static_cast<int32>(E.Rune)),
				     OffX + X, OffY + Y, HexSize * 0.8f, Glow);
			}
		}
	}
	return Pixels;
}

bool FRunikBoardRenderer::ExportGif(const rk::Puzzle& Puzzle,
                                    const std::vector<rk::Placement>& Solution,
                                    int32 MaxTicks, const FString& FilePath) const
{
	rk::Sim Sim;
	Sim.Setup(Puzzle, Solution);

	auto ToGifPixels = [](const TArray<FColor>& In)
	{
		std::vector<rk::GifPixel> Out(In.Num());
		for (int32 i = 0; i < In.Num(); ++i)
		{
			Out[i] = { In[i].R, In[i].G, In[i].B, In[i].A };
		}
		return Out;
	};

	int32 W = 0, H = 0;
	rk::GifEncoder Encoder;
	{
		const TArray<FColor> First = Render(Sim, W, H);
		Encoder.Begin(W, H);
		Encoder.AddFrame(ToGifPixels(First), 40);
	}
	for (int32 T = 0; T < MaxTicks && !Sim.IsWon(); ++T)
	{
		Sim.Tick();
		int32 FW = 0, FH = 0;
		const TArray<FColor> Frame = Render(Sim, FW, FH);
		if (FW != W || FH != H) break; // layout must stay stable
		Encoder.AddFrame(ToGifPixels(Frame), 40);
	}
	const std::vector<uint8_t>& FileBytes = Encoder.Finish();
	TArray<uint8> Out;
	Out.Append(FileBytes.data(), static_cast<int32>(FileBytes.size()));
	const bool bOk = FFileHelper::SaveArrayToFile(Out, *FilePath);
	UE_LOG(LogRunik, Log, TEXT("GIF export %s: %s"),
	       bOk ? TEXT("ok") : TEXT("FAILED"), *FilePath);
	return bOk;
}
