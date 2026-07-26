#include "RunikAssets.h"
#include "../Runik.h"
#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "TextureResource.h"

static const TCHAR* GGemNames[] = {
	TEXT("ruby"), TEXT("emerald"), TEXT("sapphire"), TEXT("amethyst"),
	TEXT("rosetear"), TEXT("citrine"), TEXT("aquamarine"), TEXT("diamond"),
	TEXT("onyx"), TEXT("smokequartz"), TEXT("starstone"), TEXT("glacier"),
	TEXT("heartstone"), TEXT("cluster"), TEXT("moonstone"), TEXT("peridot")
};

static const TCHAR* GRuneNames[] = {
	TEXT("orbit"), TEXT("push"), TEXT("pull"),
	TEXT("fuse"), TEXT("detach"), TEXT("shift")
};

FName FRunikAssets::GemSpriteName(int32 GemIndex)
{
	return FName(*FString::Printf(TEXT("gem_%s"), GGemNames[FMath::Clamp(GemIndex, 0, 15)]));
}

FName FRunikAssets::RuneSpriteName(int32 RuneIndex)
{
	return FName(*FString::Printf(TEXT("rune_%s"), GRuneNames[FMath::Clamp(RuneIndex, 0, 5)]));
}

void FRunikAssets::LoadAll(TArray<TObjectPtr<UTexture2D>>& KeepAlive)
{
	const FString Raw = FPaths::ProjectContentDir() / TEXT("Raw");

	for (int32 i = 0; i < 16; ++i)
	{
		LoadSprite(GemSpriteName(i),
			Raw / TEXT("gems") / FString::Printf(TEXT("gem_%s.png"), GGemNames[i]),
			KeepAlive);
	}
	for (int32 i = 0; i < 6; ++i)
	{
		LoadSprite(RuneSpriteName(i),
			Raw / TEXT("runes") / FString::Printf(TEXT("rune_%s.png"), GRuneNames[i]),
			KeepAlive);
	}
	static const TCHAR* Icons[] = {
		TEXT("icon_play"), TEXT("icon_pause"), TEXT("icon_step"), TEXT("icon_stop"),
		TEXT("icon_reset"), TEXT("icon_gear"), TEXT("icon_book"), TEXT("icon_gif"),
		TEXT("icon_chart"), TEXT("icon_undo"), TEXT("icon_plane"), TEXT("icon_trash")
	};
	for (const TCHAR* Icon : Icons)
	{
		LoadSprite(FName(Icon), Raw / TEXT("icons") / (FString(Icon) + TEXT(".png")), KeepAlive);
	}
	LoadSprite(TEXT("hex_base"), Raw / TEXT("hex_base.png"), KeepAlive);
	LoadSprite(TEXT("bg_board"), Raw / TEXT("bg_board.png"), KeepAlive);
	LoadSprite(TEXT("bg_menu"), Raw / TEXT("bg_menu.png"), KeepAlive);
}

bool FRunikAssets::LoadSprite(FName Name, const FString& Path,
                              TArray<TObjectPtr<UTexture2D>>& KeepAlive)
{
	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *Path))
	{
		UE_LOG(LogRunik, Warning, TEXT("Missing art file: %s"), *Path);
		return false;
	}

	IImageWrapperModule& Module =
		FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(EImageFormat::PNG);
	if (!Wrapper.IsValid() ||
	    !Wrapper->SetCompressed(FileData.GetData(), FileData.Num()))
	{
		UE_LOG(LogRunik, Warning, TEXT("Bad PNG: %s"), *Path);
		return false;
	}

	TArray<uint8> Bgra;
	if (!Wrapper->GetRaw(ERGBFormat::BGRA, 8, Bgra))
	{
		return false;
	}

	FRunikSprite Sprite;
	Sprite.Width = Wrapper->GetWidth();
	Sprite.Height = Wrapper->GetHeight();
	Sprite.Pixels.SetNumUninitialized(Sprite.Width * Sprite.Height);
	FMemory::Memcpy(Sprite.Pixels.GetData(), Bgra.GetData(),
	                Sprite.Pixels.Num() * sizeof(FColor));

	UTexture2D* Tex = UTexture2D::CreateTransient(Sprite.Width, Sprite.Height,
	                                              PF_B8G8R8A8);
	if (!Tex)
	{
		return false;
	}
	void* Mip = Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Mip, Bgra.GetData(), Bgra.Num());
	Tex->GetPlatformData()->Mips[0].BulkData.Unlock();
	Tex->SRGB = true;
	Tex->UpdateResource();
	KeepAlive.Add(Tex);

	Sprite.Texture = Tex;
	Sprite.Brush.SetResourceObject(Tex);
	Sprite.Brush.ImageSize = FVector2D(Sprite.Width, Sprite.Height);
	Sprite.Brush.DrawAs = ESlateBrushDrawType::Image;

	Sprites.Add(Name, MoveTemp(Sprite));
	return true;
}

const FRunikSprite* FRunikAssets::Get(FName Name) const
{
	return Sprites.Find(Name);
}

const FSlateBrush* FRunikAssets::GetBrush(FName Name) const
{
	const FRunikSprite* S = Sprites.Find(Name);
	return S ? &S->Brush : nullptr;
}
