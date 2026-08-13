#pragma once
#include <cstdint>
#include <string>
#include <vector>

// .8xp (TI-83/84 Plus program file) import. A .8xp stores a TI-BASIC program
// as a token stream; decoding it to our source-text form (which the
// interpreter re-tokenises per line) lets users bring in real calculator
// programs. Pure C++ — no Qt, no core_math dependency; the controller reads
// the file bytes and feeds the decoded source to ProgramStore/saveProgram.
namespace tux_ti83 {

struct Import8xpResult {
  bool ok = false;
  std::string name;    // program name decoded from the file header (raw)
  std::string source;  // decoded TI-BASIC source (newline-separated lines)
  std::string error;   // human-readable reason when !ok
  int unknownTokens = 0;  // count of tokens with no mapping (rendered as ?)
};

// Decode a .8xp file's raw bytes. Validates the signature + container layout,
// extracts the program body, and detokenises it to source text.
Import8xpResult decode8xp(const std::vector<uint8_t> &bytes);

}  // namespace tux_ti83
