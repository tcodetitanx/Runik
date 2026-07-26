// Runik core types. Engine-agnostic: standard C++17 only, so the simulation
// can be compiled and tested outside Unreal (see /Tests) and reused verbatim
// inside the game module.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rk
{

// ---------------------------------------------------------------- hex math
// Axial coordinates, flat-top hexes.
struct Hex
{
	int32_t Q = 0;
	int32_t R = 0;

	Hex() = default;
	Hex(int32_t InQ, int32_t InR) : Q(InQ), R(InR) {}

	bool operator==(const Hex& O) const { return Q == O.Q && R == O.R; }
	bool operator!=(const Hex& O) const { return !(*this == O); }
	bool operator<(const Hex& O) const { return Q != O.Q ? Q < O.Q : R < O.R; }
	Hex operator+(const Hex& O) const { return Hex(Q + O.Q, R + O.R); }
	Hex operator-(const Hex& O) const { return Hex(Q - O.Q, R - O.R); }
};

// Neighbor directions: E, NE, NW, W, SW, SE (flat-top axial).
static const Hex HexDirs[6] = {
	Hex(1, 0), Hex(1, -1), Hex(0, -1), Hex(-1, 0), Hex(-1, 1), Hex(0, 1)
};

inline int32_t HexLen(const Hex& H)
{
	const int32_t S = -H.Q - H.R;
	int32_t A = H.Q < 0 ? -H.Q : H.Q;
	int32_t B = H.R < 0 ? -H.R : H.R;
	int32_t C = S < 0 ? -S : S;
	int32_t M = A > B ? A : B;
	return M > C ? M : C;
}

inline int32_t HexDist(const Hex& A, const Hex& B) { return HexLen(A - B); }

// 60-degree rotation of a vector around the origin.
inline Hex HexRotCW(const Hex& V)  { return Hex(-V.R, V.Q + V.R); }
inline Hex HexRotCCW(const Hex& V) { return Hex(V.Q + V.R, -V.Q); }

// Rotate P around center C.
inline Hex HexRotAround(const Hex& P, const Hex& C, bool bCW)
{
	const Hex V = P - C;
	return C + (bCW ? HexRotCW(V) : HexRotCCW(V));
}

// Index of unit direction V in HexDirs, or -1 if V is not a unit direction.
inline int32_t HexDirIndex(const Hex& V)
{
	for (int32_t i = 0; i < 6; ++i)
	{
		if (HexDirs[i] == V) return i;
	}
	return -1;
}

// ---------------------------------------------------------------- entities
enum class EPlane : uint8_t
{
	Material, // rune glow: gold
	Ethereal  // rune glow: blue
};

inline EPlane OtherPlane(EPlane P)
{
	return P == EPlane::Material ? EPlane::Ethereal : EPlane::Material;
}

enum class EGem : uint8_t
{
	Ruby, Emerald, Sapphire, Amethyst,
	Rosetear, Citrine, Aquamarine, Diamond,
	Onyx, SmokeQuartz, Starstone, Glacier,
	Heartstone, Cluster, Moonstone, Peridot,
	COUNT
};

enum class ERune : uint8_t
{
	Orbit,   // Raidho:   adjacent units rotate around the rune each tick.
	         //           Sunwise (CW) on Material, widdershins (CCW) on Ethereal.
	Push,    // Thurisaz: adjacent units move one hex directly away.
	Pull,    // Nauthiz:  first unit within 3 hexes on each axis moves one hex
	         //           closer; a unit already adjacent is held in place.
	Fuse,    // Gebo:     after movement, adjacent entities that touch each
	         //           other are bonded into one compound.
	Detach,  // Kenaz:    after movement, all bonds touching adjacent entities
	         //           are severed.
	Shift,   // Dagaz:    after movement, adjacent units are flipped to the
	         //           other plane if their hexes are free there.
	COUNT
};

// Rune placement cost (the "cost" score, like Opus Magnum part cost).
inline int32_t RuneCost(ERune R)
{
	switch (R)
	{
	case ERune::Push:   return 10;
	case ERune::Pull:   return 15;
	case ERune::Orbit:  return 20;
	case ERune::Detach: return 20;
	case ERune::Fuse:   return 25;
	case ERune::Shift:  return 40;
	default:            return 0;
	}
}

inline const char* RuneName(ERune R)
{
	switch (R)
	{
	case ERune::Orbit:  return "Orbit (Raidho)";
	case ERune::Push:   return "Push (Thurisaz)";
	case ERune::Pull:   return "Pull (Nauthiz)";
	case ERune::Fuse:   return "Fuse (Gebo)";
	case ERune::Detach: return "Detach (Kenaz)";
	case ERune::Shift:  return "Shift (Dagaz)";
	default:            return "?";
	}
}

inline const char* GemName(EGem G)
{
	static const char* Names[] = {
		"Ruby", "Emerald", "Sapphire", "Amethyst",
		"Rosetear", "Citrine", "Aquamarine", "Diamond",
		"Onyx", "Smoke Quartz", "Starstone", "Glacier",
		"Heartstone", "Cluster", "Moonstone", "Peridot" };
	return Names[static_cast<int>(G)];
}

enum class EKind : uint8_t { Gem, Rune };

struct Entity
{
	int32_t Id = -1;
	EKind Kind = EKind::Gem;
	EGem Gem = EGem::Ruby;
	ERune Rune = ERune::Push;
	EPlane Plane = EPlane::Material;
	Hex Pos;
	int32_t Priority = 0;   // gems always 0; runes 1..9
	bool bStatic = false;   // puzzle fixture: acts but can never be affected
	bool bPlayerPlaced = false;
	bool bAlive = true;
};

// ---------------------------------------------------------------- puzzles
struct SpawnCell
{
	Hex Offset;             // relative to Spawner.Pos
	EGem Gem = EGem::Ruby;
};

// Spawns its gem set (bonded into one compound if multi-cell) whenever every
// cell is vacant on its plane.
struct Spawner
{
	Hex Pos;
	EPlane Plane = EPlane::Material;
	std::vector<SpawnCell> Cells;

	static Spawner Single(const Hex& P, EGem G, EPlane Pl = EPlane::Material)
	{
		Spawner S;
		S.Pos = P;
		S.Plane = Pl;
		S.Cells.push_back({ Hex(0, 0), G });
		return S;
	}
};

// Petrified gem: an inert, immovable blocker on one plane.
struct Wall
{
	Hex Pos;
	EPlane Plane = EPlane::Material;
	EGem Gem = EGem::Onyx;
};

struct OutputCell
{
	Hex Pos;
	EGem Gem = EGem::Ruby;
};

// One output zone: a compound must exactly cover the cells with matching gem
// types, on the given plane, to be consumed.
struct OutputZone
{
	std::vector<OutputCell> Cells;
	EPlane Plane = EPlane::Material;
};

struct FixedRune
{
	ERune Rune;
	Hex Pos;
	EPlane Plane = EPlane::Material;
	int32_t Priority = 1;
	bool bStatic = true;    // false: a fixture the player may displace with runes
};

struct Placement
{
	ERune Rune;
	Hex Pos;
	EPlane Plane = EPlane::Material;
	int32_t Priority = 1;
};

struct Puzzle
{
	std::string Id;
	std::string Name;
	int32_t Chapter = 0;      // 0-based
	int32_t Radius = 5;       // board radius in hexes
	int32_t RequiredOutputs = 3;
	std::vector<FixedRune> FixedRunes;
	std::vector<Wall> Walls;
	std::vector<Spawner> Spawners;
	std::vector<OutputZone> Outputs;
	uint32_t AllowedRunes = 0x3F; // bitmask over ERune
	std::string Brief;        // objective text
	std::string Lore;         // story interlude shown before the puzzle
	// Par values for the histogram comparer.
	int32_t ParCost = 0;
	int32_t ParTicks = 0;
	int32_t ParRunes = 0;
};

struct Chapter
{
	std::string Name;
	std::string Lore;
};

// ---------------------------------------------------------------- events
enum class EEventType : uint8_t
{
	Moved, Rotated, Held, Fused, Detached, Shifted, Spawned, Output, Blocked
};

struct SimEvent
{
	EEventType Type;
	int32_t EntityId = -1;
	Hex From, To;
};

// ---------------------------------------------------------------- scoring
struct Metrics
{
	int32_t Cost = 0;
	int32_t Ticks = 0;
	int32_t Runes = 0;
	int32_t Area = 0;
};

} // namespace rk
