#include "RunikPuzzles.h"
#include <map>

namespace rk
{

// ------------------------------------------------------------------ helpers
namespace
{

Puzzle Make(const char* Id, const char* Name, int32_t Chapter, int32_t Radius,
            int32_t Required, uint32_t Allowed,
            const char* Brief, const char* Lore)
{
	Puzzle P;
	P.Id = Id;
	P.Name = Name;
	P.Chapter = Chapter;
	P.Radius = Radius;
	P.RequiredOutputs = Required;
	P.AllowedRunes = Allowed;
	P.Brief = Brief;
	P.Lore = Lore;
	return P;
}

OutputZone Zone1(int32_t Q, int32_t R, EGem G, EPlane Pl = EPlane::Material)
{
	OutputZone Z;
	Z.Plane = Pl;
	Z.Cells.push_back({ Hex(Q, R), G });
	return Z;
}

} // namespace

// ----------------------------------------------------------------- chapters
const std::vector<Chapter>& GetChapters()
{
	static const std::vector<Chapter> Chapters = {
	{ "First Marks",
	  "Jarnvik, in the year the sun dimmed.\n\n"
	  "You are Eir Runasdottir, last apprentice of the Runewrights' Guild. "
	  "Master Alviss is gone - his workshop cold, his journals open to a page "
	  "that reads only: 'The stone the sky forgot. I have gone to fetch it.'\n\n"
	  "The guild-hall answers to inscribed runes, not to hands. If you are to "
	  "follow him, you must first learn to make the stones obey the old marks." },
	{ "The Two Courts",
	  "Alviss' second journal speaks of two worlds laid one atop the other: "
	  "the Gold Court of the Material, where runes burn like embers, and the "
	  "Azure Court of the Ethereal, where they glow cold as drowned stars.\n\n"
	  "'What is heavy here is weightless there,' he writes. 'The wheel that "
	  "turns sunwise in the day-world turns widdershins in the night-world. "
	  "Dagaz is the door. Learn to knock.'" },
	{ "Order of Precedence",
	  "The third journal is scorched at the edges.\n\n"
	  "'The runes contend. Each mark believes itself the rightful speaker, "
	  "and where two speak at once, the stone hears only silence. Give your "
	  "runes rank, as the gods ranked the Aesir. The greater voice commands "
	  "the lesser - and a rune may move a rune, as a king moves a jarl.'" },
	{ "Sundering and Union",
	  "'Gebo is a gift given both ways,' the fourth journal says. 'Two stones "
	  "made one, one stone made two. All smithing is only this.\n\n"
	  "The Aetherstone is not found. It is COMPOSED. I understand now why the "
	  "guild burned this page in every other copy.'" },
	{ "The Aetherstone",
	  "The last entry is written in a shaking hand.\n\n"
	  "'I have crossed to the Azure Court and cannot return - the door needs "
	  "a key, and the key is the stone itself. Celestial Aetherstone: forged "
	  "of sea, of light, of moon, fused in the day-world and quenched in the "
	  "night. Eir - if you read this - forge it. Open the door. Bring me home.'" },
	};
	return Chapters;
}

// ------------------------------------------------------------------ puzzles
const std::vector<Puzzle>& GetPuzzles()
{
	static std::vector<Puzzle> Puzzles;
	if (!Puzzles.empty()) return Puzzles;

	// ---- Chapter 1: First Marks -------------------------------------------
	{
		Puzzle P = Make("c1p1", "The First Stone", 0, 3, 3,
			RuneBit(ERune::Pull),
			"Deliver 3 rubies to the collector. Nauthiz, the rune of need, "
			"draws stones along its six axes and holds fast what stands beside it.",
			"The training hall's smallest board. Chalk dust, and your master's "
			"empty chair. Begin as every runewright begins: with need.");
		P.Spawners.push_back(Spawner::Single(Hex(-2, 0), EGem::Ruby));
		P.Outputs.push_back(Zone1(0, 0, EGem::Ruby));
		P.ParCost = 15; P.ParTicks = 8; P.ParRunes = 1;
		Puzzles.push_back(P);
	}
	{
		Puzzle P = Make("c1p2", "The Ward", 0, 3, 3,
			RuneBit(ERune::Pull) | RuneBit(ERune::Push),
			"The collector stands off the axis of need. Thurisaz, the thorn, "
			"drives away all that stands beside it - and the greater rank "
			"speaks over the lesser.",
			"'Pull alone makes a hoard,' Alviss wrote. 'Push alone makes an "
			"enemy. Together they make a road.'");
		P.Spawners.push_back(Spawner::Single(Hex(-2, 0), EGem::Citrine));
		P.Outputs.push_back(Zone1(-2, 1, EGem::Citrine));
		P.ParCost = 25; P.ParTicks = 12; P.ParRunes = 2;
		Puzzles.push_back(P);
	}
	{
		Puzzle P = Make("c1p3", "Turning the Wheel", 0, 3, 3,
			RuneBit(ERune::Pull) | RuneBit(ERune::Orbit),
			"Raidho is the journey: whatever stands beside it is carried "
			"around it, sunwise. Deliver 3 emeralds around the bend.",
			"The wheel-rune was carved on the guild's door. 'All things "
			"return,' it means. You hope that is true of masters, too.");
		P.Spawners.push_back(Spawner::Single(Hex(-2, 0), EGem::Emerald));
		P.Outputs.push_back(Zone1(0, 1, EGem::Emerald));
		P.ParCost = 35; P.ParTicks = 12; P.ParRunes = 2;
		Puzzles.push_back(P);
	}

	// ---- Chapter 2: The Two Courts ----------------------------------------
	{
		Puzzle P = Make("c2p1", "Thin Places", 1, 4, 3,
			RuneBit(ERune::Pull) | RuneBit(ERune::Shift),
			"A wall of petrified stone bars the way - but only in the "
			"day-world. Dagaz opens the door: deliver 3 rubies to the "
			"ethereal collector beyond the wall.",
			"You find the first thin place behind the guild's forge, where "
			"the air ripples like water over hot iron.");
		for (int32_t R = -4; R <= 4; ++R)
		{
			if (HexLen(Hex(0, R)) <= 4) P.Walls.push_back({ Hex(0, R), EPlane::Material, EGem::Onyx });
		}
		P.Spawners.push_back(Spawner::Single(Hex(-4, 0), EGem::Ruby));
		P.Outputs.push_back(Zone1(0, 0, EGem::Ruby, EPlane::Ethereal));
		P.ParCost = 70; P.ParTicks = 16; P.ParRunes = 3;
		Puzzles.push_back(P);
	}
	{
		Puzzle P = Make("c2p2", "Widdershins", 1, 4, 3,
			RuneBit(ERune::Pull) | RuneBit(ERune::Orbit),
			"In the Azure Court the wheel turns the other way. Deliver 3 "
			"moonstones - every rune here burns blue, and Raidho carries "
			"widdershins.",
			"On the far side of the thin place, sound arrives late and "
			"colors hum. Your own runes glow a cold, patient blue.");
		P.Spawners.push_back(Spawner::Single(Hex(2, 0), EGem::Moonstone, EPlane::Ethereal));
		P.Outputs.push_back(Zone1(0, -1, EGem::Moonstone, EPlane::Ethereal));
		P.ParCost = 35; P.ParTicks = 16; P.ParRunes = 2;
		Puzzles.push_back(P);
	}
	{
		Puzzle P = Make("c2p3", "The Loom", 1, 4, 6,
			RuneBit(ERune::Pull) | RuneBit(ERune::Shift),
			"Two threads, two worlds. The ruby must be delivered in the "
			"Ethereal; the moonstone in the Material. Weave the streams "
			"across the veil without tangling them.",
			"'The Courts trade,' writes Alviss. 'Day-iron for night-silver. "
			"The runewright is the loom on which the trade is woven.'");
		P.Spawners.push_back(Spawner::Single(Hex(-2, 0), EGem::Ruby));
		P.Spawners.push_back(Spawner::Single(Hex(-2, 2), EGem::Moonstone, EPlane::Ethereal));
		P.Outputs.push_back(Zone1(2, 0, EGem::Ruby, EPlane::Ethereal));
		P.Outputs.push_back(Zone1(1, 2, EGem::Moonstone));
		P.ParCost = 140; P.ParTicks = 24; P.ParRunes = 6;
		Puzzles.push_back(P);
	}

	// ---- Chapter 3: Order of Precedence -----------------------------------
	{
		Puzzle P = Make("c3p1", "The Greater Voice", 2, 4, 3,
			RuneBit(ERune::Pull),
			"An ancient wheel-rune of rank 2 spins the amethysts endlessly. "
			"Only a greater voice can call them out of the dance.",
			"The guild's cellar holds machines still running after a hundred "
			"years. 'Do not stop them,' says the sign. 'Outrank them.'");
		P.FixedRunes.push_back({ ERune::Orbit, Hex(0, 0), EPlane::Material, 2, true });
		P.Spawners.push_back(Spawner::Single(Hex(-1, 1), EGem::Amethyst));
		P.Outputs.push_back(Zone1(2, 0, EGem::Amethyst));
		P.ParCost = 15; P.ParTicks = 14; P.ParRunes = 1;
		Puzzles.push_back(P);
	}
	{
		Puzzle P = Make("c3p2", "Mend the Wheel", 2, 4, 3,
			RuneBit(ERune::Pull),
			"A rune of need has been knocked out of its socket - two hexes "
			"north of where the old machine needs it. A rune may move a "
			"rune: drag it home with a greater voice.",
			"Frost heave, or sabotage? The journal only says: 'What the "
			"mountain moves, the runewright moves back.'");
		P.FixedRunes.push_back({ ERune::Pull, Hex(2, 2), EPlane::Material, 1, false });
		P.Spawners.push_back(Spawner::Single(Hex(-1, 0), EGem::Ruby));
		P.Outputs.push_back(Zone1(1, 0, EGem::Ruby));
		P.ParCost = 15; P.ParTicks = 16; P.ParRunes = 1;
		Puzzles.push_back(P);
	}
	{
		Puzzle P = Make("c3p3", "Gear Train", 2, 5, 3,
			RuneBit(ERune::Orbit),
			"Three wheels, each outranking the last, hand the stone from "
			"ring to ring across the whole board. Deliver 3 emeralds.",
			"'Rank is not vanity,' Alviss notes. 'It is the order in which "
			"the world listens.'");
		P.Spawners.push_back(Spawner::Single(Hex(-3, 0), EGem::Emerald));
		P.Outputs.push_back(Zone1(3, 0, EGem::Emerald));
		P.ParCost = 60; P.ParTicks = 20; P.ParRunes = 3;
		Puzzles.push_back(P);
	}

	// ---- Chapter 4: Sundering and Union -----------------------------------
	{
		Puzzle P = Make("c4p1", "The Gift", 3, 4, 3,
			RuneBit(ERune::Pull) | RuneBit(ERune::Fuse),
			"Gebo joins what touches beside it. Hold the ruby and the "
			"sapphire together and give the gift: 3 fused pairs.",
			"Two streams, one stone. The oldest wedding-rite of the guild, "
			"performed on gems instead of souls.");
		P.Spawners.push_back(Spawner::Single(Hex(-3, 0), EGem::Ruby));
		P.Spawners.push_back(Spawner::Single(Hex(0, -3), EGem::Sapphire));
		OutputZone Z;
		Z.Cells.push_back({ Hex(-1, 0), EGem::Ruby });
		Z.Cells.push_back({ Hex(0, -1), EGem::Sapphire });
		P.Outputs.push_back(Z);
		P.ParCost = 40; P.ParTicks = 10; P.ParRunes = 2;
		Puzzles.push_back(P);
	}
	{
		Puzzle P = Make("c4p2", "Sundered Pair", 3, 4, 6,
			RuneBit(ERune::Pull) | RuneBit(ERune::Detach),
			"The mine sends its stones welded in pairs, but the collector "
			"accepts only single stones. Kenaz, the torch, opens every "
			"bond beside it: sunder the pairs before they arrive.",
			"'To open is also a craft,' says the journal. 'Ask any egg.'");
		Spawner S;
		S.Pos = Hex(-3, 0);
		S.Cells.push_back({ Hex(0, 0), EGem::Ruby });
		S.Cells.push_back({ Hex(1, 0), EGem::Sapphire });
		P.Spawners.push_back(S);
		P.Outputs.push_back(Zone1(0, 0, EGem::Ruby));
		P.Outputs.push_back(Zone1(0, 0, EGem::Sapphire));
		P.ParCost = 35; P.ParTicks = 16; P.ParRunes = 2;
		Puzzles.push_back(P);
	}
	{
		Puzzle P = Make("c4p3", "The Third Shard", 3, 4, 3,
			RuneBit(ERune::Pull) | RuneBit(ERune::Fuse),
			"Three streams meet at one rune of need. Two gifts bind them "
			"into an arc of three. Deliver 3 completed arcs.",
			"Ruby for the hearth, citrine for the harvest, peridot for the "
			"spring. The old year-blessing, in stone.");
		P.Spawners.push_back(Spawner::Single(Hex(-3, 0), EGem::Ruby));
		P.Spawners.push_back(Spawner::Single(Hex(0, -3), EGem::Citrine));
		P.Spawners.push_back(Spawner::Single(Hex(3, -3), EGem::Peridot));
		OutputZone Z;
		Z.Cells.push_back({ Hex(-1, 0), EGem::Ruby });
		Z.Cells.push_back({ Hex(0, -1), EGem::Citrine });
		Z.Cells.push_back({ Hex(1, -1), EGem::Peridot });
		P.Outputs.push_back(Z);
		P.ParCost = 65; P.ParTicks = 10; P.ParRunes = 3;
		Puzzles.push_back(P);
	}

	// ---- Chapter 5: The Aetherstone ---------------------------------------
	{
		Puzzle P = Make("c5p1", "Gold and Azure", 4, 4, 3,
			RuneBit(ERune::Pull) | RuneBit(ERune::Fuse) | RuneBit(ERune::Shift),
			"Forge in the day-world, quench in the night: fuse starstone "
			"to glacier, then shift the pair across the veil to the "
			"ethereal collector.",
			"The forge-fire burns gold; the quench-water gleams blue. You "
			"begin to see why the stone must be made in both worlds.");
		P.Spawners.push_back(Spawner::Single(Hex(-3, 0), EGem::Starstone));
		P.Spawners.push_back(Spawner::Single(Hex(0, -3), EGem::Glacier));
		OutputZone Z;
		Z.Plane = EPlane::Ethereal;
		Z.Cells.push_back({ Hex(-1, 0), EGem::Starstone });
		Z.Cells.push_back({ Hex(0, -1), EGem::Glacier });
		P.Outputs.push_back(Z);
		P.ParCost = 80; P.ParTicks = 12; P.ParRunes = 3;
		Puzzles.push_back(P);
	}
	{
		Puzzle P = Make("c5p2", "The Quickened Wheel", 4, 5, 3,
			RuneBit(ERune::Orbit) | RuneBit(ERune::Shift),
			"Two ranked wheels carry the peridot across the board; at the "
			"far side of the second ring, the door opens. Deliver 3 "
			"peridots to the ethereal collector.",
			"'Speed is a kind of key,' the journal insists. 'The door "
			"opens for what arrives still turning.'");
		P.Spawners.push_back(Spawner::Single(Hex(-3, 0), EGem::Peridot));
		P.Outputs.push_back(Zone1(1, 0, EGem::Peridot, EPlane::Ethereal));
		P.ParCost = 80; P.ParTicks = 20; P.ParRunes = 3;
		Puzzles.push_back(P);
	}
	{
		Puzzle P = Make("c5p3", "The Celestial Aetherstone", 4, 5, 5,
			RuneBit(ERune::Pull) | RuneBit(ERune::Fuse) | RuneBit(ERune::Shift),
			"Sea, light and moon: sapphire, diamond, moonstone. Fuse the "
			"three into the sacred arc and quench it in the Ethereal. "
			"Forge 5 Aetherstones to open the door.",
			"The last page of the last journal is blank except for five "
			"small hexes, drawn in a careful, hopeful hand.\n\n"
			"Bring him home.");
		P.Spawners.push_back(Spawner::Single(Hex(-3, 0), EGem::Sapphire));
		P.Spawners.push_back(Spawner::Single(Hex(0, -3), EGem::Diamond));
		P.Spawners.push_back(Spawner::Single(Hex(3, -3), EGem::Moonstone));
		OutputZone Z;
		Z.Plane = EPlane::Ethereal;
		Z.Cells.push_back({ Hex(-1, 0), EGem::Sapphire });
		Z.Cells.push_back({ Hex(0, -1), EGem::Diamond });
		Z.Cells.push_back({ Hex(1, -1), EGem::Moonstone });
		P.Outputs.push_back(Z);
		P.ParCost = 105; P.ParTicks = 14; P.ParRunes = 4;
		Puzzles.push_back(P);
	}

	return Puzzles;
}

const Puzzle* FindPuzzle(const std::string& Id)
{
	for (const Puzzle& P : GetPuzzles())
	{
		if (P.Id == Id) return &P;
	}
	return nullptr;
}

// -------------------------------------------------------------- solutions
std::vector<Placement> GetReferenceSolution(const std::string& Id)
{
	using V = std::vector<Placement>;
	static const std::map<std::string, V> Solutions = {
	{ "c1p1", V{
		{ ERune::Pull, Hex(1, 0), EPlane::Material, 1 } } },
	{ "c1p2", V{
		{ ERune::Pull, Hex(1, 0), EPlane::Material, 1 },
		{ ERune::Push, Hex(0, -1), EPlane::Material, 2 } } },
	{ "c1p3", V{
		{ ERune::Pull, Hex(1, 0), EPlane::Material, 1 },
		{ ERune::Orbit, Hex(-1, 1), EPlane::Material, 2 } } },
	{ "c2p1", V{
		{ ERune::Pull, Hex(-1, 0), EPlane::Material, 1 },
		{ ERune::Shift, Hex(-2, 1), EPlane::Material, 1 },
		{ ERune::Pull, Hex(1, 0), EPlane::Ethereal, 1 } } },
	{ "c2p2", V{
		{ ERune::Pull, Hex(-1, 0), EPlane::Ethereal, 1 },
		{ ERune::Orbit, Hex(1, -1), EPlane::Ethereal, 2 } } },
	{ "c2p3", V{
		{ ERune::Pull, Hex(1, 0), EPlane::Material, 1 },
		{ ERune::Shift, Hex(0, 1), EPlane::Material, 1 },
		{ ERune::Pull, Hex(3, 0), EPlane::Ethereal, 1 },
		{ ERune::Pull, Hex(1, 2), EPlane::Ethereal, 1 },
		{ ERune::Shift, Hex(-1, 3), EPlane::Ethereal, 1 },
		{ ERune::Pull, Hex(2, 2), EPlane::Material, 1 } } },
	{ "c3p1", V{
		{ ERune::Pull, Hex(3, 0), EPlane::Material, 3 } } },
	{ "c3p2", V{
		{ ERune::Pull, Hex(2, -1), EPlane::Material, 2 } } },
	{ "c3p3", V{
		{ ERune::Orbit, Hex(-2, 0), EPlane::Material, 1 },
		{ ERune::Orbit, Hex(0, 0), EPlane::Material, 2 },
		{ ERune::Orbit, Hex(2, 0), EPlane::Material, 3 } } },
	{ "c4p1", V{
		{ ERune::Pull, Hex(0, 0), EPlane::Material, 1 },
		{ ERune::Fuse, Hex(-1, -1), EPlane::Material, 1 } } },
	{ "c4p2", V{
		{ ERune::Pull, Hex(1, 0), EPlane::Material, 1 },
		{ ERune::Detach, Hex(0, -1), EPlane::Material, 1 } } },
	{ "c4p3", V{
		{ ERune::Pull, Hex(0, 0), EPlane::Material, 1 },
		{ ERune::Fuse, Hex(-1, -1), EPlane::Material, 1 },
		{ ERune::Fuse, Hex(1, -2), EPlane::Material, 1 } } },
	{ "c5p1", V{
		{ ERune::Pull, Hex(0, 0), EPlane::Material, 1 },
		{ ERune::Fuse, Hex(-1, -1), EPlane::Material, 1 },
		{ ERune::Shift, Hex(-1, 1), EPlane::Material, 1 } } },
	{ "c5p2", V{
		{ ERune::Orbit, Hex(-2, 0), EPlane::Material, 1 },
		{ ERune::Orbit, Hex(0, 0), EPlane::Material, 2 },
		{ ERune::Shift, Hex(2, 0), EPlane::Material, 1 } } },
	{ "c5p3", V{
		{ ERune::Pull, Hex(0, 0), EPlane::Material, 1 },
		{ ERune::Fuse, Hex(-1, -1), EPlane::Material, 1 },
		{ ERune::Fuse, Hex(1, -2), EPlane::Material, 1 },
		{ ERune::Shift, Hex(-1, 1), EPlane::Material, 1 } } },
	};
	auto It = Solutions.find(Id);
	return It != Solutions.end() ? It->second : V();
}

} // namespace rk
