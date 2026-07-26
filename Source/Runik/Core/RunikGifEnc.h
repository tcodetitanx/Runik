// Self-contained GIF89a encoder (LZW, global 3-3-2 palette, infinite loop).
// Engine-agnostic so it can be verified by the standalone tests.
#pragma once

#include <cstdint>
#include <map>
#include <vector>

namespace rk
{

struct GifPixel
{
	uint8_t R = 0, G = 0, B = 0, A = 255;
};

class GifEncoder
{
public:
	void Begin(int32_t Width, int32_t Height);
	void AddFrame(const std::vector<GifPixel>& Pixels, int32_t DelayCentiseconds);
	// Appends the trailer and returns the complete file bytes.
	const std::vector<uint8_t>& Finish();

private:
	int32_t W = 0, H = 0;
	bool bFinished = false;
	std::vector<uint8_t> Bytes;

	void WriteLzwImage(const std::vector<uint8_t>& Indexed);
};

} // namespace rk
