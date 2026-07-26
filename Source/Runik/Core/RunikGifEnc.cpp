#include "RunikGifEnc.h"

namespace rk
{

namespace
{

inline uint8_t Quant332(const GifPixel& C)
{
	return static_cast<uint8_t>((C.R & 0xE0) | ((C.G & 0xE0) >> 3) | (C.B >> 6));
}

struct BitPacker
{
	std::vector<uint8_t>& Out;
	uint32_t Acc = 0;
	int32_t Bits = 0;
	std::vector<uint8_t> Block;

	explicit BitPacker(std::vector<uint8_t>& InOut) : Out(InOut) {}

	void Put(uint32_t Code, int32_t CodeBits)
	{
		Acc |= Code << Bits;
		Bits += CodeBits;
		while (Bits >= 8)
		{
			Block.push_back(static_cast<uint8_t>(Acc & 0xFF));
			Acc >>= 8;
			Bits -= 8;
			if (Block.size() == 255) Flush();
		}
	}

	void Flush()
	{
		if (!Block.empty())
		{
			Out.push_back(static_cast<uint8_t>(Block.size()));
			Out.insert(Out.end(), Block.begin(), Block.end());
			Block.clear();
		}
	}

	void FinishBlocks()
	{
		if (Bits > 0)
		{
			Block.push_back(static_cast<uint8_t>(Acc & 0xFF));
			Acc = 0;
			Bits = 0;
			if (Block.size() == 255) Flush();
		}
		Flush();
		Out.push_back(0); // block terminator
	}
};

} // namespace

void GifEncoder::Begin(int32_t Width, int32_t Height)
{
	W = Width;
	H = Height;
	bFinished = false;
	Bytes.clear();
	const char* Sig = "GIF89a";
	Bytes.insert(Bytes.end(), Sig, Sig + 6);
	Bytes.push_back(W & 0xFF); Bytes.push_back((W >> 8) & 0xFF);
	Bytes.push_back(H & 0xFF); Bytes.push_back((H >> 8) & 0xFF);
	Bytes.push_back(0xF7); // global color table, 8 bpp
	Bytes.push_back(0);
	Bytes.push_back(0);
	for (int32_t i = 0; i < 256; ++i)
	{
		uint8_t R = static_cast<uint8_t>(i & 0xE0);
		uint8_t G = static_cast<uint8_t>((i & 0x1C) << 3);
		uint8_t B = static_cast<uint8_t>((i & 0x03) << 6);
		Bytes.push_back(R | (R >> 3) | (R >> 6));
		Bytes.push_back(G | (G >> 3) | (G >> 6));
		Bytes.push_back(B | (B >> 2) | (B >> 4) | (B >> 6));
	}
	static const uint8_t Loop[] = {
		0x21, 0xFF, 0x0B, 'N','E','T','S','C','A','P','E','2','.','0',
		0x03, 0x01, 0x00, 0x00, 0x00 };
	Bytes.insert(Bytes.end(), Loop, Loop + sizeof(Loop));
}

void GifEncoder::AddFrame(const std::vector<GifPixel>& Pixels, int32_t DelayCS)
{
	// graphics control extension
	Bytes.push_back(0x21); Bytes.push_back(0xF9); Bytes.push_back(0x04);
	Bytes.push_back(0x00);
	Bytes.push_back(DelayCS & 0xFF); Bytes.push_back((DelayCS >> 8) & 0xFF);
	Bytes.push_back(0x00); Bytes.push_back(0x00);
	// image descriptor
	Bytes.push_back(0x2C);
	Bytes.push_back(0); Bytes.push_back(0); Bytes.push_back(0); Bytes.push_back(0);
	Bytes.push_back(W & 0xFF); Bytes.push_back((W >> 8) & 0xFF);
	Bytes.push_back(H & 0xFF); Bytes.push_back((H >> 8) & 0xFF);
	Bytes.push_back(0x00);

	std::vector<uint8_t> Indexed(Pixels.size());
	for (size_t i = 0; i < Pixels.size(); ++i)
	{
		Indexed[i] = Quant332(Pixels[i]);
	}
	WriteLzwImage(Indexed);
}

void GifEncoder::WriteLzwImage(const std::vector<uint8_t>& Indexed)
{
	Bytes.push_back(8); // min LZW code size
	BitPacker Packer(Bytes);

	const int32_t ClearCode = 256;
	const int32_t EoiCode = 257;
	std::map<uint64_t, int32_t> Dict;
	int32_t NextCode = 258;
	int32_t CodeBits = 9;

	Packer.Put(ClearCode, CodeBits);
	int32_t Prefix = -1;
	for (uint8_t Pixel : Indexed)
	{
		if (Prefix < 0)
		{
			Prefix = Pixel;
			continue;
		}
		const uint64_t Key = (static_cast<uint64_t>(Prefix) << 8) | Pixel;
		auto It = Dict.find(Key);
		if (It != Dict.end())
		{
			Prefix = It->second;
		}
		else
		{
			Packer.Put(Prefix, CodeBits);
			Dict[Key] = NextCode;
			if (NextCode == (1 << CodeBits) && CodeBits < 12)
			{
				++CodeBits;
			}
			++NextCode;
			if (NextCode >= 4095)
			{
				Packer.Put(ClearCode, CodeBits);
				Dict.clear();
				NextCode = 258;
				CodeBits = 9;
			}
			Prefix = Pixel;
		}
	}
	if (Prefix >= 0)
	{
		Packer.Put(Prefix, CodeBits);
	}
	Packer.Put(EoiCode, CodeBits);
	Packer.FinishBlocks();
}

const std::vector<uint8_t>& GifEncoder::Finish()
{
	if (!bFinished)
	{
		Bytes.push_back(0x3B);
		bFinished = true;
	}
	return Bytes;
}

} // namespace rk
