#pragma once
#include <charconv>
#include <cstdint>
#include <locale>
#include <random>
#include <sstream>
#include <string>
#include <string_view>

namespace dywx::mvp {
// MSVC's engine extractor throws on malformed/truncated input even when the
// stream has exceptions disabled. Validate before calling it in exception-free UE.
inline bool ReadRandomState(std::string_view text, std::mt19937& result) {
  if (text.empty() || text.size() > 20000) return false;
  // The standard leaves stream representation implementation-defined: MSVC has
  // 624 state words; libstdc++ also writes the current position. Keep native saves.
  static const std::size_t expectedWords = [] {
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << std::mt19937{};
    std::istringstream input(output.str());
    input.imbue(std::locale::classic());
    std::uint64_t word = 0;
    std::size_t count = 0;
    while (input >> word) ++count;
    return count;
  }();
  std::size_t count = 0;
  const char* cursor = text.data();
  const char* end = cursor + text.size();
  auto whitespace = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
  while (cursor != end) {
    while (cursor != end && whitespace(*cursor)) ++cursor;
    if (cursor == end) break;
    if (*cursor < '0' || *cursor > '9' || count >= expectedWords) return false;
    std::uint64_t value = 0;
    const auto parsed = std::from_chars(cursor, end, value);
    if (parsed.ec != std::errc{} || (parsed.ptr != end && !whitespace(*parsed.ptr))) return false;
    const auto maximum = count < std::mt19937::state_size ? std::uint64_t(std::mt19937::max()) : std::uint64_t(std::mt19937::state_size);
    if (value > maximum) return false;
    ++count;
    cursor = parsed.ptr;
  }
  if (count != expectedWords) return false;
  std::istringstream input{std::string(text)};
  input.imbue(std::locale::classic());
  std::mt19937 candidate;
  if (!(input >> candidate)) return false;
  input >> std::ws;
  if (!input.eof()) return false;
  result = candidate;
  return true;
}
}
