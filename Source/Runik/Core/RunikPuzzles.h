// Campaign content: chapters, puzzles, lore, and authored reference
// solutions (used by the automated tests and the in-game hint seer).
#pragma once

#include "RunikCore.h"

namespace rk
{

const std::vector<Chapter>& GetChapters();
const std::vector<Puzzle>& GetPuzzles();
const Puzzle* FindPuzzle(const std::string& Id);

// The developer solution that is guaranteed (by tests) to solve the puzzle.
std::vector<Placement> GetReferenceSolution(const std::string& PuzzleId);

inline uint32_t RuneBit(ERune R) { return 1u << static_cast<uint32_t>(R); }

} // namespace rk
