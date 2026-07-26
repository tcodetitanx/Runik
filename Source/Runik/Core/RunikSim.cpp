#include "RunikSim.h"
#include <algorithm>

namespace rk
{

// ------------------------------------------------------------------- setup
void Sim::Setup(const Puzzle& InPuzzle, const std::vector<Placement>& InPlacements)
{
	ThePuzzle = InPuzzle;
	Entities.clear();
	Bonds.clear();
	LastEvents.clear();
	VisitedHexes.clear();
	NextId = 0;
	TickCount = 0;
	OutputsDone = 0;
	PlacedRuneCount = 0;
	PlacedRuneCost = 0;

	for (const FixedRune& F : ThePuzzle.FixedRunes)
	{
		Entity& E = Spawn(EKind::Rune, F.Pos, F.Plane);
		E.Rune = F.Rune;
		E.Priority = F.Priority;
		E.bStatic = F.bStatic;
	}
	for (const Wall& W : ThePuzzle.Walls)
	{
		Entity& E = Spawn(EKind::Gem, W.Pos, W.Plane);
		E.Gem = W.Gem;
		E.bStatic = true;
	}
	for (const Placement& P : InPlacements)
	{
		Entity& E = Spawn(EKind::Rune, P.Pos, P.Plane);
		E.Rune = P.Rune;
		E.Priority = P.Priority < 1 ? 1 : (P.Priority > 9 ? 9 : P.Priority);
		E.bPlayerPlaced = true;
		++PlacedRuneCount;
		PlacedRuneCost += RuneCost(P.Rune);
	}
	RunSpawners();
	LastEvents.clear();
}

Entity& Sim::Spawn(EKind Kind, const Hex& Pos, EPlane Plane)
{
	Entity E;
	E.Id = NextId++;
	E.Kind = Kind;
	E.Pos = Pos;
	E.Plane = Plane;
	Entities.push_back(E);
	VisitedHexes.insert({ Pos.Q, Pos.R });
	return Entities.back();
}

const Entity* Sim::EntityAt(const Hex& H, EPlane P) const
{
	for (const Entity& E : Entities)
	{
		if (E.bAlive && E.Plane == P && E.Pos == H) return &E;
	}
	return nullptr;
}

Entity* Sim::EntityAt(const Hex& H, EPlane P)
{
	return const_cast<Entity*>(static_cast<const Sim*>(this)->EntityAt(H, P));
}

// ------------------------------------------------------------------- units
std::vector<Sim::Unit> Sim::BuildUnits() const
{
	// Union-find over alive entities through bonds. Statics are always alone.
	std::map<int32_t, int32_t> Parent;
	for (const Entity& E : Entities)
	{
		if (E.bAlive) Parent[E.Id] = E.Id;
	}
	struct Find
	{
		std::map<int32_t, int32_t>& P;
		int32_t operator()(int32_t X)
		{
			while (P[X] != X) { P[X] = P[P[X]]; X = P[X]; }
			return X;
		}
	} FindRoot{ Parent };

	for (const auto& B : Bonds)
	{
		if (Parent.count(B.first) && Parent.count(B.second))
		{
			Parent[FindRoot(B.first)] = FindRoot(B.second);
		}
	}

	std::map<int32_t, int32_t> RootToUnit;
	std::vector<Unit> Units;
	for (const Entity& E : Entities)
	{
		if (!E.bAlive) continue;
		const int32_t Root = FindRoot(E.Id);
		auto It = RootToUnit.find(Root);
		if (It == RootToUnit.end())
		{
			RootToUnit[Root] = static_cast<int32_t>(Units.size());
			Units.push_back(Unit());
			It = RootToUnit.find(Root);
		}
		Unit& U = Units[It->second];
		U.Ids.push_back(E.Id);
		U.Plane = E.Plane;
		if (E.Priority > U.Priority) U.Priority = E.Priority;
		if (E.bStatic) U.bStatic = true;
	}
	return Units;
}

int32_t Sim::UnitIndexOfEntity(const std::vector<Unit>& Units, int32_t EntityId) const
{
	for (size_t i = 0; i < Units.size(); ++i)
	{
		for (int32_t Id : Units[i].Ids)
		{
			if (Id == EntityId) return static_cast<int32_t>(i);
		}
	}
	return -1;
}

// ----------------------------------------------------------------- effects
bool Sim::SameTransform(const Effect& A, const Effect& B) const
{
	if (A.bRotation != B.bRotation) return false;
	if (A.bRotation) return A.Center == B.Center && A.bCW == B.bCW;
	return A.Delta == B.Delta;
}

Hex Sim::Apply(const Effect& E, const Hex& H) const
{
	return E.bRotation ? HexRotAround(H, E.Center, E.bCW) : H + E.Delta;
}

void Sim::CollectEffects(const std::vector<Unit>& Units,
                         std::vector<std::vector<Effect>>& OutPerUnit) const
{
	OutPerUnit.assign(Units.size(), {});

	for (const Entity& R : Entities)
	{
		if (!R.bAlive || R.Kind != EKind::Rune) continue;
		const int32_t SelfUnit = UnitIndexOfEntity(Units, R.Id);

		auto Affectable = [&](int32_t UnitIdx) -> bool
		{
			if (UnitIdx < 0 || UnitIdx == SelfUnit) return false;
			const Unit& U = Units[UnitIdx];
			return !U.bStatic && U.Priority < R.Priority;
		};

		if (R.Rune == ERune::Push)
		{
			for (int32_t d = 0; d < 6; ++d)
			{
				const Hex N = R.Pos + HexDirs[d];
				const Entity* T = EntityAt(N, R.Plane);
				if (!T) continue;
				const int32_t UI = UnitIndexOfEntity(Units, T->Id);
				if (!Affectable(UI)) continue;
				Effect E;
				E.SourceRuneId = R.Id;
				E.SourcePriority = R.Priority;
				E.Delta = HexDirs[d];
				OutPerUnit[UI].push_back(E);
			}
		}
		else if (R.Rune == ERune::Pull)
		{
			for (int32_t d = 0; d < 6; ++d)
			{
				for (int32_t k = 1; k <= 3; ++k)
				{
					const Hex H = Hex(R.Pos.Q + HexDirs[d].Q * k,
					                  R.Pos.R + HexDirs[d].R * k);
					if (!InBounds(H)) break;
					const Entity* T = EntityAt(H, R.Plane);
					if (!T) continue;
					const int32_t UI = UnitIndexOfEntity(Units, T->Id);
					if (Affectable(UI))
					{
						Effect E;
						E.SourceRuneId = R.Id;
						E.SourcePriority = R.Priority;
						// adjacent: held; farther: drawn one hex closer
						E.Delta = (k == 1) ? Hex(0, 0)
						                   : Hex(-HexDirs[d].Q, -HexDirs[d].R);
						OutPerUnit[UI].push_back(E);
					}
					break; // first entity on the ray blocks the rest
				}
			}
		}
		else if (R.Rune == ERune::Orbit)
		{
			for (int32_t d = 0; d < 6; ++d)
			{
				const Hex N = R.Pos + HexDirs[d];
				const Entity* T = EntityAt(N, R.Plane);
				if (!T) continue;
				const int32_t UI = UnitIndexOfEntity(Units, T->Id);
				if (!Affectable(UI)) continue;
				Effect E;
				E.SourceRuneId = R.Id;
				E.SourcePriority = R.Priority;
				E.bRotation = true;
				E.Center = R.Pos;
				E.bCW = (R.Plane == EPlane::Material); // sunwise / widdershins
				OutPerUnit[UI].push_back(E);
			}
		}
		// Fuse / Detach / Shift act after movement, not here.
	}
}

// ---------------------------------------------------------------- movement
void Sim::ResolveAndMove(std::vector<Unit>& Units,
                         std::vector<std::vector<Effect>>& PerUnit)
{
	const size_t N = Units.size();

	// Pick one effect per unit: highest priority wins, ties must agree.
	std::vector<const Effect*> Chosen(N, nullptr);
	for (size_t i = 0; i < N; ++i)
	{
		int32_t Best = -1;
		for (const Effect& E : PerUnit[i])
		{
			if (E.SourcePriority > Best) Best = E.SourcePriority;
		}
		if (Best < 0) continue;
		const Effect* Pick = nullptr;
		bool bConflict = false;
		for (const Effect& E : PerUnit[i])
		{
			if (E.SourcePriority != Best) continue;
			if (!Pick) Pick = &E;
			else if (!SameTransform(*Pick, E)) bConflict = true;
		}
		if (!bConflict) Chosen[i] = Pick;
	}

	// Current and proposed hex sets.
	std::vector<std::vector<Hex>> Cur(N), Tgt(N);
	std::vector<bool> Moving(N, false);
	for (size_t i = 0; i < N; ++i)
	{
		for (int32_t Id : Units[i].Ids) Cur[i].push_back(Entities[Id].Pos);
		Tgt[i] = Cur[i];
		if (Chosen[i] && !(!Chosen[i]->bRotation && Chosen[i]->Delta == Hex(0, 0)))
		{
			for (Hex& H : Tgt[i]) H = Apply(*Chosen[i], H);
			Moving[i] = Tgt[i] != Cur[i];
		}
		else if (Chosen[i])
		{
			LastEvents.push_back({ EEventType::Held, Units[i].Ids[0],
			                       Cur[i][0], Cur[i][0] });
		}
	}

	auto Overlap = [](const std::vector<Hex>& A, const std::vector<Hex>& B)
	{
		for (const Hex& X : A)
			for (const Hex& Y : B)
				if (X == Y) return true;
		return false;
	};

	// Iteratively cancel invalid moves until stable.
	bool bChanged = true;
	while (bChanged)
	{
		bChanged = false;
		for (size_t i = 0; i < N; ++i)
		{
			if (!Moving[i]) continue;
			bool bCancel = false;
			for (const Hex& H : Tgt[i])
			{
				if (!InBounds(H)) { bCancel = true; break; }
			}
			for (size_t j = 0; j < N && !bCancel; ++j)
			{
				if (j == i || Units[j].Plane != Units[i].Plane) continue;
				const std::vector<Hex>& Other = Moving[j] ? Tgt[j] : Cur[j];
				if (Overlap(Tgt[i], Other))
				{
					// Trains are fine: moving into a hex the other unit is
					// leaving. Only cancel on real overlap of final hexes.
					bCancel = true;
				}
				else if (Moving[j] && Overlap(Tgt[i], Cur[j]) &&
				         Overlap(Tgt[j], Cur[i]))
				{
					bCancel = true; // mutual pass-through
				}
			}
			if (bCancel)
			{
				Moving[i] = false;
				Tgt[i] = Cur[i];
				bChanged = true;
				LastEvents.push_back({ EEventType::Blocked, Units[i].Ids[0],
				                       Cur[i][0], Cur[i][0] });
			}
		}
	}

	for (size_t i = 0; i < N; ++i)
	{
		if (!Moving[i]) continue;
		for (size_t k = 0; k < Units[i].Ids.size(); ++k)
		{
			Entity& E = Entities[Units[i].Ids[k]];
			const Hex From = E.Pos;
			E.Pos = Tgt[i][k];
			VisitedHexes.insert({ E.Pos.Q, E.Pos.R });
			LastEvents.push_back({ Chosen[i]->bRotation ? EEventType::Rotated
			                                            : EEventType::Moved,
			                       E.Id, From, E.Pos });
		}
	}
}

// -------------------------------------------------------------- post moves
void Sim::ApplyFuse()
{
	for (const Entity& R : Entities)
	{
		if (!R.bAlive || R.Kind != EKind::Rune || R.Rune != ERune::Fuse) continue;
		std::vector<const Entity*> Near;
		for (int32_t d = 0; d < 6; ++d)
		{
			const Entity* T = EntityAt(R.Pos + HexDirs[d], R.Plane);
			if (T && T->bAlive && !T->bStatic && T->Id != R.Id) Near.push_back(T);
		}
		for (size_t a = 0; a < Near.size(); ++a)
		{
			for (size_t b = a + 1; b < Near.size(); ++b)
			{
				if (HexDist(Near[a]->Pos, Near[b]->Pos) != 1) continue;
				const int32_t Lo = std::min(Near[a]->Id, Near[b]->Id);
				const int32_t Hi = std::max(Near[a]->Id, Near[b]->Id);
				bool bHave = false;
				for (const auto& Bd : Bonds)
				{
					if (Bd.first == Lo && Bd.second == Hi) { bHave = true; break; }
				}
				if (!bHave)
				{
					Bonds.push_back({ Lo, Hi });
					LastEvents.push_back({ EEventType::Fused, Lo,
					                       Near[a]->Pos, Near[b]->Pos });
				}
			}
		}
	}
}

void Sim::ApplyDetach()
{
	for (const Entity& R : Entities)
	{
		if (!R.bAlive || R.Kind != EKind::Rune || R.Rune != ERune::Detach) continue;
		std::set<int32_t> Near;
		for (int32_t d = 0; d < 6; ++d)
		{
			const Entity* T = EntityAt(R.Pos + HexDirs[d], R.Plane);
			if (T) Near.insert(T->Id);
		}
		for (size_t i = Bonds.size(); i-- > 0;)
		{
			if (Near.count(Bonds[i].first) || Near.count(Bonds[i].second))
			{
				LastEvents.push_back({ EEventType::Detached, Bonds[i].first,
				                       Entities[Bonds[i].first].Pos,
				                       Entities[Bonds[i].second].Pos });
				Bonds.erase(Bonds.begin() + i);
			}
		}
	}
}

void Sim::ApplyShift()
{
	std::vector<Unit> Units = BuildUnits();
	std::vector<bool> Done(Units.size(), false);
	for (const Entity& R : Entities)
	{
		if (!R.bAlive || R.Kind != EKind::Rune || R.Rune != ERune::Shift) continue;
		const int32_t SelfUnit = UnitIndexOfEntity(Units, R.Id);
		for (int32_t d = 0; d < 6; ++d)
		{
			const Entity* T = EntityAt(R.Pos + HexDirs[d], R.Plane);
			if (!T) continue;
			const int32_t UI = UnitIndexOfEntity(Units, T->Id);
			if (UI < 0 || UI == SelfUnit || Done[UI]) continue;
			const Unit& U = Units[UI];
			if (U.bStatic || U.Priority >= R.Priority) continue;
			const EPlane Dest = OtherPlane(U.Plane);
			bool bFree = true;
			for (int32_t Id : U.Ids)
			{
				if (EntityAt(Entities[Id].Pos, Dest)) { bFree = false; break; }
			}
			if (!bFree) continue;
			for (int32_t Id : U.Ids)
			{
				Entities[Id].Plane = Dest;
				LastEvents.push_back({ EEventType::Shifted, Id,
				                       Entities[Id].Pos, Entities[Id].Pos });
			}
			Done[UI] = true;
		}
	}
}

// ----------------------------------------------------------------- outputs
void Sim::CheckOutputs()
{
	std::vector<Unit> Units = BuildUnits();
	for (const OutputZone& Z : ThePuzzle.Outputs)
	{
		int32_t ZoneUnit = -1;
		bool bMatch = true;
		for (const OutputCell& C : Z.Cells)
		{
			const Entity* E = EntityAt(C.Pos, Z.Plane);
			if (!E || E->Kind != EKind::Gem || E->Gem != C.Gem || E->bStatic)
			{
				bMatch = false;
				break;
			}
			const int32_t UI = UnitIndexOfEntity(Units, E->Id);
			if (ZoneUnit < 0) ZoneUnit = UI;
			else if (ZoneUnit != UI) { bMatch = false; break; }
		}
		if (!bMatch || ZoneUnit < 0) continue;
		// The compound must cover the zone exactly - no extra hanging atoms.
		if (Units[ZoneUnit].Ids.size() != Z.Cells.size()) continue;

		for (int32_t Id : Units[ZoneUnit].Ids)
		{
			Entities[Id].bAlive = false;
			LastEvents.push_back({ EEventType::Output, Id,
			                       Entities[Id].Pos, Entities[Id].Pos });
			for (size_t i = Bonds.size(); i-- > 0;)
			{
				if (Bonds[i].first == Id || Bonds[i].second == Id)
				{
					Bonds.erase(Bonds.begin() + i);
				}
			}
		}
		++OutputsDone;
	}
}

void Sim::RunSpawners()
{
	for (const Spawner& S : ThePuzzle.Spawners)
	{
		bool bFree = true;
		for (const SpawnCell& C : S.Cells)
		{
			if (EntityAt(S.Pos + C.Offset, S.Plane)) { bFree = false; break; }
		}
		if (!bFree) continue;

		std::vector<int32_t> NewIds;
		for (const SpawnCell& C : S.Cells)
		{
			Entity& E = Spawn(EKind::Gem, S.Pos + C.Offset, S.Plane);
			E.Gem = C.Gem;
			NewIds.push_back(E.Id);
			LastEvents.push_back({ EEventType::Spawned, E.Id, E.Pos, E.Pos });
		}
		// bond every touching pair so multi-cell spawns arrive as one compound
		for (size_t a = 0; a < NewIds.size(); ++a)
		{
			for (size_t b = a + 1; b < NewIds.size(); ++b)
			{
				if (HexDist(Entities[NewIds[a]].Pos, Entities[NewIds[b]].Pos) == 1)
				{
					Bonds.push_back({ NewIds[a], NewIds[b] });
				}
			}
		}
	}
}

// -------------------------------------------------------------------- tick
void Sim::Tick()
{
	LastEvents.clear();

	std::vector<Unit> Units = BuildUnits();
	std::vector<std::vector<Effect>> PerUnit;
	CollectEffects(Units, PerUnit);
	ResolveAndMove(Units, PerUnit);
	ApplyFuse();
	ApplyDetach();
	ApplyShift();
	CheckOutputs();
	RunSpawners();
	++TickCount;
}

Metrics Sim::GetMetrics() const
{
	Metrics M;
	M.Cost = PlacedRuneCost;
	M.Ticks = TickCount;
	M.Runes = PlacedRuneCount;
	M.Area = static_cast<int32_t>(VisitedHexes.size());
	return M;
}

} // namespace rk
