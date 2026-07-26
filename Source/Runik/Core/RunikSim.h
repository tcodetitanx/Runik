// The Runik machine simulation. Deterministic, tick-based.
#pragma once

#include "RunikCore.h"
#include <map>
#include <set>

namespace rk
{

class Sim
{
public:
	// Set up a fresh board from a puzzle plus the player's rune placements.
	void Setup(const Puzzle& InPuzzle, const std::vector<Placement>& InPlacements);

	// Advance one tick. Returns true while the machine may still make progress.
	void Tick();

	bool IsWon() const { return OutputsDone >= ThePuzzle.RequiredOutputs; }

	const Puzzle& GetPuzzle() const { return ThePuzzle; }
	const std::vector<Entity>& GetEntities() const { return Entities; }
	const std::vector<SimEvent>& GetLastEvents() const { return LastEvents; }
	const std::vector<std::pair<int32_t, int32_t>>& GetBonds() const { return Bonds; }
	int32_t GetTicks() const { return TickCount; }
	int32_t GetOutputsDone() const { return OutputsDone; }
	Metrics GetMetrics() const;

	bool InBounds(const Hex& H) const { return HexLen(H) <= ThePuzzle.Radius; }
	const Entity* EntityAt(const Hex& H, EPlane P) const;
	Entity* EntityAt(const Hex& H, EPlane P);

private:
	// A rigid unit: one entity or one bonded compound (single plane).
	struct Unit
	{
		std::vector<int32_t> Ids;
		EPlane Plane = EPlane::Material;
		int32_t Priority = 0;   // max over members
		bool bStatic = false;
	};

	// Candidate transform for a unit this tick.
	struct Effect
	{
		int32_t SourceRuneId = -1;
		int32_t SourcePriority = 0;
		bool bRotation = false;
		Hex Delta;      // translation (0,0 = hold)
		Hex Center;     // rotation center
		bool bCW = true;
	};

	Puzzle ThePuzzle;
	std::vector<Entity> Entities;
	std::vector<std::pair<int32_t, int32_t>> Bonds; // entity id pairs
	std::vector<SimEvent> LastEvents;
	int32_t NextId = 0;
	int32_t TickCount = 0;
	int32_t OutputsDone = 0;
	int32_t PlacedRuneCount = 0;
	int32_t PlacedRuneCost = 0;
	std::set<std::pair<int32_t, int32_t>> VisitedHexes; // area metric

	Entity& Spawn(EKind Kind, const Hex& Pos, EPlane Plane);
	std::vector<Unit> BuildUnits() const;
	bool SameTransform(const Effect& A, const Effect& B) const;
	Hex Apply(const Effect& E, const Hex& H) const;
	void CollectEffects(const std::vector<Unit>& Units,
	                    std::vector<std::vector<Effect>>& OutPerUnit) const;
	void ResolveAndMove(std::vector<Unit>& Units,
	                    std::vector<std::vector<Effect>>& PerUnit);
	void ApplyFuse();
	void ApplyDetach();
	void ApplyShift();
	void CheckOutputs();
	void RunSpawners();
	int32_t UnitIndexOfEntity(const std::vector<Unit>& Units, int32_t EntityId) const;
};

} // namespace rk
