// Standalone verification of the Runik core simulation - compiled with the
// plain VS toolchain, no Unreal required. Exercises every rune mechanic and
// proves each campaign puzzle is solvable by its reference solution.
#include "../Source/Runik/Core/RunikCore.h"
#include "../Source/Runik/Core/RunikSim.h"
#include "../Source/Runik/Core/RunikPuzzles.h"
#include "../Source/Runik/Core/RunikGifEnc.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace rk;

static int GTests = 0;
static int GFails = 0;

#define CHECK(Cond, Msg)                                                      \
	do {                                                                      \
		++GTests;                                                             \
		if (!(Cond)) { ++GFails; std::printf("FAIL: %s (%s:%d)\n", Msg,       \
			__FILE__, __LINE__); }                                            \
	} while (0)

static void DumpBoard(const Sim& S)
{
	std::printf("  tick=%d outputs=%d\n", S.GetTicks(), S.GetOutputsDone());
	for (const Entity& E : S.GetEntities())
	{
		if (!E.bAlive) continue;
		std::printf("   #%d %s %s (%d,%d) %s prio=%d%s\n", E.Id,
			E.Kind == EKind::Gem ? "gem " : "rune",
			E.Kind == EKind::Gem ? GemName(E.Gem) : RuneName(E.Rune),
			E.Pos.Q, E.Pos.R,
			E.Plane == EPlane::Material ? "MAT" : "ETH",
			E.Priority, E.bStatic ? " static" : "");
	}
}

// --------------------------------------------------------------- hex math
static void TestHexMath()
{
	CHECK(HexDist(Hex(0, 0), Hex(3, -3)) == 3, "hex distance NE");
	CHECK(HexDist(Hex(-2, 1), Hex(2, -1)) == 4, "hex distance mixed");
	CHECK(HexRotCW(Hex(1, 0)) == Hex(0, 1), "rot cw E->SE");
	CHECK(HexRotCCW(Hex(1, 0)) == Hex(1, -1), "rot ccw E->NE");
	Hex P(2, -1);
	Hex Q = P;
	for (int i = 0; i < 6; ++i) Q = HexRotCW(Q);
	CHECK(Q == P, "six cw rotations = identity");
	CHECK(HexRotAround(Hex(1, 0), Hex(0, 0), true) == Hex(0, 1), "rotate around origin");
}

// --------------------------------------------------------------- mechanics
static Puzzle Arena(int32_t Radius = 5)
{
	Puzzle P;
	P.Id = "arena";
	P.Radius = Radius;
	P.RequiredOutputs = 999;
	return P;
}

static void TestPush()
{
	Puzzle P = Arena();
	P.Spawners.push_back(Spawner::Single(Hex(1, 0), EGem::Ruby));
	Sim S;
	S.Setup(P, { { ERune::Push, Hex(0, 0), EPlane::Material, 1 } });
	S.Tick();
	const Entity* G = S.EntityAt(Hex(2, 0), EPlane::Material);
	CHECK(G && G->Kind == EKind::Gem, "push moves adjacent gem away");
	S.Tick();
	CHECK(S.EntityAt(Hex(2, 0), EPlane::Material) != nullptr ||
	      S.EntityAt(Hex(3, 0), EPlane::Material) != nullptr,
	      "gem exists after second tick");
	// gem at range 2 is out of push influence, must still be at (2,0)...
	// except the respawned gem at (1,0) was pushed into it - it stays blocked.
	CHECK(S.EntityAt(Hex(3, 0), EPlane::Material) == nullptr,
	      "push only affects adjacent hexes");
}

static void TestPullAndHold()
{
	Puzzle P = Arena();
	P.Spawners.push_back(Spawner::Single(Hex(-3, 0), EGem::Ruby));
	Sim S;
	S.Setup(P, { { ERune::Pull, Hex(0, 0), EPlane::Material, 1 } });
	S.Tick();
	CHECK(S.EntityAt(Hex(-2, 0), EPlane::Material) != nullptr, "pull draws gem closer");
	S.Tick();
	CHECK(S.EntityAt(Hex(-1, 0), EPlane::Material) != nullptr, "pull draws to adjacency");
	S.Tick();
	CHECK(S.EntityAt(Hex(-1, 0), EPlane::Material) != nullptr, "adjacent gem is held");
	CHECK(S.EntityAt(Hex(0, 0), EPlane::Material)->Kind == EKind::Rune,
	      "rune hex never entered");
}

static void TestPullRange()
{
	Puzzle P = Arena(6);
	P.Spawners.push_back(Spawner::Single(Hex(-5, 0), EGem::Ruby));
	Sim S;
	S.Setup(P, { { ERune::Pull, Hex(0, 0), EPlane::Material, 1 } });
	S.Tick();
	CHECK(S.EntityAt(Hex(-5, 0), EPlane::Material) != nullptr,
	      "gems beyond range 3 are not pulled");
}

static void TestOrbit()
{
	Puzzle P = Arena();
	P.Spawners.push_back(Spawner::Single(Hex(1, 0), EGem::Ruby));
	Sim S;
	S.Setup(P, { { ERune::Orbit, Hex(0, 0), EPlane::Material, 1 } });
	// Material plane: sunwise (CW). (1,0) -> (0,1) -> (-1,1) -> ...
	const Hex Ring[6] = { Hex(1,0), Hex(0,1), Hex(-1,1), Hex(-1,0), Hex(0,-1), Hex(1,-1) };
	// After the first tick the spawner refills, so track the first gem by id 1.
	for (int i = 1; i <= 6; ++i)
	{
		S.Tick();
		const Entity& G = S.GetEntities()[1];
		CHECK(G.Pos == Ring[i % 6], "orbit CW ring position");
	}
}

static void TestOrbitEtherealCCW()
{
	Puzzle P = Arena();
	P.Spawners.push_back(Spawner::Single(Hex(1, 0), EGem::Ruby, EPlane::Ethereal));
	Sim S;
	S.Setup(P, { { ERune::Orbit, Hex(0, 0), EPlane::Ethereal, 1 } });
	S.Tick();
	const Entity& G = S.GetEntities()[1];
	CHECK(G.Pos == Hex(1, -1), "ethereal orbit turns widdershins (CCW)");
}

static void TestPriorityImmunity()
{
	// A pull rune tries to drag another rune of equal rank: nothing happens.
	Puzzle P = Arena();
	P.FixedRunes.push_back({ ERune::Push, Hex(-2, 0), EPlane::Material, 1, false });
	Sim S;
	S.Setup(P, { { ERune::Pull, Hex(0, 0), EPlane::Material, 1 } });
	S.Tick();
	CHECK(S.EntityAt(Hex(-2, 0), EPlane::Material) != nullptr,
	      "equal priority rune is immune");

	// Rank 2 pull beats the rank 1 rune: it gets dragged.
	Sim S2;
	S2.Setup(P, { { ERune::Pull, Hex(0, 0), EPlane::Material, 2 } });
	S2.Tick();
	CHECK(S2.EntityAt(Hex(-1, 0), EPlane::Material) != nullptr,
	      "higher priority rune drags lesser rune");
}

static void TestConflictCancels()
{
	// Two equal pushes from opposite sides: the gem freezes.
	Puzzle P = Arena();
	P.Spawners.push_back(Spawner::Single(Hex(0, 0), EGem::Ruby));
	Sim S;
	S.Setup(P, {
		{ ERune::Push, Hex(-1, 0), EPlane::Material, 1 },
		{ ERune::Push, Hex(1, 0), EPlane::Material, 1 } });
	S.Tick();
	const Entity* G = S.EntityAt(Hex(0, 0), EPlane::Material);
	CHECK(G && G->Kind == EKind::Gem, "conflicting equal effects cancel");
}

static void TestPlanesAreSeparate()
{
	Puzzle P = Arena();
	P.Spawners.push_back(Spawner::Single(Hex(1, 0), EGem::Ruby, EPlane::Ethereal));
	Sim S;
	S.Setup(P, { { ERune::Push, Hex(0, 0), EPlane::Material, 1 } });
	S.Tick();
	CHECK(S.EntityAt(Hex(1, 0), EPlane::Ethereal) != nullptr,
	      "material rune ignores ethereal gem");
}

static void TestFuseDetach()
{
	Puzzle P = Arena();
	P.Spawners.push_back(Spawner::Single(Hex(-1, 0), EGem::Ruby));
	P.Spawners.push_back(Spawner::Single(Hex(0, -1), EGem::Sapphire));
	Sim S;
	S.Setup(P, { { ERune::Fuse, Hex(-1, -1), EPlane::Material, 1 } });
	S.Tick();
	CHECK(S.GetBonds().size() == 1, "fuse bonds touching neighbors");

	// Bonded pair moves rigidly: push one, both move.
	Puzzle P2 = Arena();
	P2.Spawners.push_back(Spawner::Single(Hex(1, 0), EGem::Ruby));
	P2.Spawners.push_back(Spawner::Single(Hex(2, 0), EGem::Sapphire));
	Sim S2;
	S2.Setup(P2, {
		{ ERune::Fuse, Hex(2, -1), EPlane::Material, 1 }, // touches both gems
		{ ERune::Push, Hex(0, 0), EPlane::Material, 1 } });
	S2.Tick(); // push blocked (rear gem in the way), fuse bonds the pair
	S2.Tick(); // pair pushed as one unit to (2,0)+(3,0), leaving push range
	const Entity* A = S2.EntityAt(Hex(2, 0), EPlane::Material);
	const Entity* B = S2.EntityAt(Hex(3, 0), EPlane::Material);
	CHECK(A && B && A->Kind == EKind::Gem && B->Kind == EKind::Gem,
	      "bonded pair moves as one unit");
}

static void TestShift()
{
	Puzzle P = Arena();
	P.Spawners.push_back(Spawner::Single(Hex(1, 0), EGem::Ruby));
	Sim S;
	S.Setup(P, { { ERune::Shift, Hex(0, 0), EPlane::Material, 1 } });
	S.Tick();
	CHECK(S.EntityAt(Hex(1, 0), EPlane::Ethereal) != nullptr,
	      "shift flips adjacent gem to the ethereal plane");
	// The spawner hex (material) is now free: a new gem spawns, and next tick
	// it flips too - but the ethereal hex is occupied, so it must wait.
	S.Tick();
	CHECK(S.EntityAt(Hex(1, 0), EPlane::Material) != nullptr,
	      "second gem blocked from shifting while destination occupied");
}

static void TestBlocking()
{
	Puzzle P = Arena();
	P.Walls.push_back({ Hex(2, 0), EPlane::Material, EGem::Onyx });
	P.Spawners.push_back(Spawner::Single(Hex(1, 0), EGem::Ruby));
	Sim S;
	S.Setup(P, { { ERune::Push, Hex(0, 0), EPlane::Material, 1 } });
	S.Tick();
	CHECK(S.EntityAt(Hex(1, 0), EPlane::Material)->Kind == EKind::Gem,
	      "gem blocked by wall stays put");
}

static void TestBoardEdge()
{
	Puzzle P = Arena(2);
	P.Spawners.push_back(Spawner::Single(Hex(2, 0), EGem::Ruby));
	Sim S;
	S.Setup(P, { { ERune::Push, Hex(1, 0), EPlane::Material, 1 } });
	S.Tick();
	CHECK(S.EntityAt(Hex(2, 0), EPlane::Material)->Kind == EKind::Gem,
	      "gem cannot be pushed off the board");
}

// ---------------------------------------------------------------- puzzles
static void TestAllPuzzleSolutions()
{
	const int32_t MaxTicks = 120;
	for (const Puzzle& P : GetPuzzles())
	{
		std::vector<Placement> Sol = GetReferenceSolution(P.Id);
		++GTests;
		if (Sol.empty())
		{
			++GFails;
			std::printf("FAIL: no reference solution for %s\n", P.Id.c_str());
			continue;
		}
		Sim S;
		S.Setup(P, Sol);
		int32_t T = 0;
		while (!S.IsWon() && T < MaxTicks) { S.Tick(); ++T; }
		if (!S.IsWon())
		{
			++GFails;
			std::printf("FAIL: puzzle %s (%s) unsolved after %d ticks\n",
				P.Id.c_str(), P.Name.c_str(), MaxTicks);
			DumpBoard(S);
		}
		else
		{
			std::printf("  ok: %-6s %-28s solved in %3d ticks "
				"(cost %d, runes %d)\n",
				P.Id.c_str(), P.Name.c_str(), T,
				S.GetMetrics().Cost, S.GetMetrics().Runes);
		}
	}
}

// -------------------------------------------------------------------- gif
static void TestGifEncoder()
{
	// Animated test card: an orbiting bright square over a gradient.
	const int32_t W = 120, H = 90;
	GifEncoder Enc;
	Enc.Begin(W, H);
	for (int f = 0; f < 12; ++f)
	{
		std::vector<GifPixel> Px(W * H);
		for (int y = 0; y < H; ++y)
			for (int x = 0; x < W; ++x)
				Px[y * W + x] = { uint8_t(x * 2), uint8_t(y * 2), 60, 255 };
		const int cx = 60 + int(35 * std::cos(f * 3.14159 / 6)),
		          cy = 45 + int(30 * std::sin(f * 3.14159 / 6));
		for (int y = cy - 8; y < cy + 8; ++y)
			for (int x = cx - 8; x < cx + 8; ++x)
				if (x >= 0 && y >= 0 && x < W && y < H)
					Px[y * W + x] = { 255, 210, 80, 255 };
		Enc.AddFrame(Px, 8);
	}
	const std::vector<uint8_t>& Bytes = Enc.Finish();
	CHECK(Bytes.size() > 800, "gif has content");
	CHECK(Bytes[0] == 'G' && Bytes[1] == 'I' && Bytes[5] == 'a', "gif signature");
	CHECK(Bytes.back() == 0x3B, "gif trailer");
	FILE* F = nullptr;
	fopen_s(&F, "Tests\\bin\\test.gif", "wb");
	if (!F) fopen_s(&F, "bin\\test.gif", "wb");
	if (F)
	{
		fwrite(Bytes.data(), 1, Bytes.size(), F);
		fclose(F);
		std::printf("  wrote test.gif (%zu bytes)\n", Bytes.size());
	}
}

int main()
{
	std::printf("Runik core tests\n");
	TestHexMath();
	TestPush();
	TestPullAndHold();
	TestPullRange();
	TestOrbit();
	TestOrbitEtherealCCW();
	TestPriorityImmunity();
	TestConflictCancels();
	TestPlanesAreSeparate();
	TestFuseDetach();
	TestShift();
	TestBlocking();
	TestBoardEdge();
	TestGifEncoder();
	TestAllPuzzleSolutions();
	std::printf("%d checks, %d failures\n", GTests, GFails);
	return GFails == 0 ? 0 : 1;
}
