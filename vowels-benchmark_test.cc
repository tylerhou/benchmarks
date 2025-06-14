#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <random>
#include <string>

#include "benchmark/benchmark.h"

static constexpr std::string_view kVowels = "aeiouAEIOU";

static bool HasVowelLoop(std::string_view haystack) {
  for (char v : kVowels) {
    for (int i = 0; i < haystack.size(); ++i) {
      if (haystack[i] == v) {
        return true;
      }
    }
  }
  return false;
}

static bool HasVowelLoopInterchanged(std::string_view haystack) {
  for (int i = 0; i < haystack.size(); ++i) {
    for (char v : kVowels) {
      if (haystack[i] == v) {
        return true;
      }
    }
  }
  return false;
}

// Initialize regex table. I did not verify that the table is actually correct;
// but the performance of the code should not depend on the table entries (as
// long as the entries are not trivial).
static constexpr char kCharMin = std::numeric_limits<char>::min();
static constexpr char kCharMax = std::numeric_limits<char>::max();
static constexpr int kSize = kCharMax - kCharMin + 1;

static constexpr int kReject = 0;
static constexpr int kAccept = 1;
static constexpr std::array<std::array<int, kSize>, 2> MakeRegexTable() {
  std::array<std::array<int, kSize>, 2> tbl = {};

  // Important to use int here so we don't overflow char
  for (int c = kCharMin; c <= kCharMax; ++c) {
    // char might be signed, so need to make sure idx starts at 0.
    int idx = c - kCharMin;
    if (kVowels.find(static_cast<char>(c)) != -1) {
      tbl[kReject][idx] = kAccept;
    } else {
      tbl[kReject][idx] = kReject;
    }
    tbl[kAccept][idx] = kAccept;
  }

  return tbl;
}

static constexpr auto kRegexTable = MakeRegexTable();

static bool HasVowelRegex(std::string_view haystack) {
  int state = kReject;
  for (auto c : haystack) {
    state = kRegexTable[state][c];
  }

  return state == kAccept;
}

static bool HasVowelRegexEarlyReturn(std::string_view haystack) {
  int state = kReject;
  for (auto c : haystack) {
    state = kRegexTable[state][c];
    if (state == kAccept) {
      return true;
    }
  }

  return false;
}

static constexpr std::string_view kCharsWithVowels =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static constexpr std::string_view kCharsNoVowels =
    "0123456789"
    "bcdfghjklmnpqrstvwxyz"
    "BCDFGHJKLMNPQRSTVWXYZ";

static constexpr int kShortNumStrings = 1'000;
static constexpr int kLongNumStrings = 1'000;

static constexpr auto ShortStringDistribution(std::mt19937& rng) {
  return [&]() {
    std::binomial_distribution<> d(15, 0.5);
    return d(rng) + 5;
  };
}

static constexpr auto LongStringDistribution(std::mt19937& rng) {
  return [&]() {
    std::binomial_distribution<> d(10000, 0.5);
    return d(rng);
  };
}

template <typename T>
static std::unique_ptr<std::vector<std::string>> MakeStrings(
    std::mt19937& rng, std::string_view characters, int num_strings,
    T string_length) {
  auto strs = std::make_unique<std::vector<std::string>>();
  strs->reserve(num_strings);

  std::uniform_int_distribution<> char_dist(0, characters.size() - 1);
  auto generate_one = [&]() -> std::string {
    std::string result;
    std::generate_n(std::back_inserter(result), string_length(),
                    [&]() { return characters[char_dist(rng)]; });
    return result;
  };

  std::generate_n(std::back_inserter(*strs), num_strings, generate_one);

  return strs;
}

static std::vector<std::string>& ShortWithVowels() {
  static std::vector<std::string>* strs = nullptr;
  if (strs == nullptr) {
    std::random_device dev;
    std::mt19937 rng(dev());
    strs = MakeStrings(rng, kCharsWithVowels, /*num_strings=*/kShortNumStrings,
                       ShortStringDistribution(rng))
               .release();
  }

  return *strs;
}

static std::vector<std::string>& ShortNoVowels() {
  static std::vector<std::string>* strs = nullptr;
  if (strs == nullptr) {
    std::random_device dev;
    std::mt19937 rng(dev());
    strs = MakeStrings(rng, kCharsNoVowels, /*num_strings=*/kShortNumStrings,
                       ShortStringDistribution(rng))
               .release();
  }

  return *strs;
}
static std::vector<std::string>& LongWithVowels() {
  static std::vector<std::string>* strs = nullptr;
  if (strs == nullptr) {
    std::random_device dev;
    std::mt19937 rng(dev());
    strs = MakeStrings(rng, kCharsWithVowels, /*num_strings=*/kLongNumStrings,
                       LongStringDistribution(rng))
               .release();
  }

  return *strs;
}

static std::vector<std::string>& LongNoVowels() {
  static std::vector<std::string>* strs = nullptr;
  if (strs == nullptr) {
    std::random_device dev;
    std::mt19937 rng(dev());
    strs = MakeStrings(rng, kCharsNoVowels, /*num_strings=*/kLongNumStrings,
                       LongStringDistribution(rng))
               .release();
  }

  return *strs;
}

#define BENCHMARK_HAS_VOWEL(Fn, Data, Args...)                              \
  static void BM_##Fn##_##Data(benchmark::State& state) {                   \
    auto strs = Data();                                                     \
    for (auto _ : state) {                                                  \
      for (const std::string& s : strs) benchmark::DoNotOptimize(Fn(Args)); \
    }                                                                       \
  }                                                                         \
                                                                            \
  BENCHMARK(BM_##Fn##_##Data);

BENCHMARK_HAS_VOWEL(HasVowelLoop, ShortWithVowels, s)
BENCHMARK_HAS_VOWEL(HasVowelLoopInterchanged, ShortWithVowels, s)
BENCHMARK_HAS_VOWEL(HasVowelRegex, ShortWithVowels, s)
BENCHMARK_HAS_VOWEL(HasVowelRegexEarlyReturn, ShortWithVowels, s)

BENCHMARK_HAS_VOWEL(HasVowelLoop, ShortNoVowels, s)
BENCHMARK_HAS_VOWEL(HasVowelLoopInterchanged, ShortNoVowels, s)
BENCHMARK_HAS_VOWEL(HasVowelRegex, ShortNoVowels, s)
BENCHMARK_HAS_VOWEL(HasVowelRegexEarlyReturn, ShortNoVowels, s)

BENCHMARK_HAS_VOWEL(HasVowelLoop, LongWithVowels, s)
BENCHMARK_HAS_VOWEL(HasVowelLoopInterchanged, LongWithVowels, s)
BENCHMARK_HAS_VOWEL(HasVowelRegex, LongWithVowels, s)
BENCHMARK_HAS_VOWEL(HasVowelRegexEarlyReturn, LongWithVowels, s)

BENCHMARK_HAS_VOWEL(HasVowelLoop, LongNoVowels, s)
BENCHMARK_HAS_VOWEL(HasVowelLoopInterchanged, LongNoVowels, s)
BENCHMARK_HAS_VOWEL(HasVowelRegex, LongNoVowels, s)
BENCHMARK_HAS_VOWEL(HasVowelRegexEarlyReturn, LongNoVowels, s)

/* Results on my M1 Mac:

tylerhou@ ~/code/benchmarks main*
❯ bazel run -c opt :vowels-benchmark_test
INFO: Analyzed target //:vowels-benchmark_test (0 packages loaded, 0 targets configured).
INFO: Found 1 target...
Target //:vowels-benchmark_test up-to-date:
  bazel-bin/vowels-benchmark_test
INFO: Elapsed time: 1.117s, Critical Path: 1.04s
INFO: 3 processes: 1 internal, 2 processwrapper-sandbox.
INFO: Build completed successfully, 3 total actions
INFO: Running command line: external/bazel_tools/tools/test/test-setup.sh ./vowels-benchmark_test
exec ${PAGER:-/usr/bin/less} "$0" || exit 1
Executing tests from //:vowels-benchmark_test
-----------------------------------------------------------------------------
2025-06-14T12:21:55+00:00
Running /home/tylerhou/.cache/bazel/_bazel_tylerhou/09f0f2c0c9650554e1952e274ae6605c/execroot/_main/bazel-out/aarch64-opt/bin/vowels-benchmark_test.runfiles/_main/vowels-benchmark_test
Run on (4 X 48 MHz CPU s)
CPU Caches:
  L1 Data 128 KiB (x4)
  L1 Instruction 192 KiB (x4)
  L2 Unified 12288 KiB (x4)
Load Average: 0.43, 0.28, 0.24
--------------------------------------------------------------------------------------
Benchmark                                            Time             CPU   Iterations
--------------------------------------------------------------------------------------
BM_HasVowelLoop_ShortWithVowels                  20113 ns        20106 ns        33958
BM_HasVowelLoopInterchanged_ShortWithVowels       4086 ns         4084 ns       171375
BM_HasVowelRegex_ShortWithVowels                  9051 ns         9046 ns        75526
BM_HasVowelRegexEarlyReturn_ShortWithVowels       4623 ns         4621 ns       152526

BM_HasVowelLoop_ShortNoVowels                    49324 ns        49323 ns        14188
BM_HasVowelLoopInterchanged_ShortNoVowels        11568 ns        11566 ns        57688
BM_HasVowelRegex_ShortNoVowels                    8974 ns         8972 ns        72943
BM_HasVowelRegexEarlyReturn_ShortNoVowels        17387 ns        17385 ns        36878

BM_HasVowelLoop_LongWithVowels                   31715 ns        31703 ns        21966
BM_HasVowelLoopInterchanged_LongWithVowels        4960 ns         4958 ns       138642
BM_HasVowelRegex_LongWithVowels               10934954 ns     10922601 ns           64
BM_HasVowelRegexEarlyReturn_LongWithVowels        6102 ns         6099 ns       113229

BM_HasVowelLoop_LongNoVowels                  16585685 ns     16579396 ns           42
BM_HasVowelLoopInterchanged_LongNoVowels       3229223 ns      3227815 ns          217
BM_HasVowelRegex_LongNoVowels                 11001319 ns     10996631 ns           64
BM_HasVowelRegexEarlyReturn_LongNoVowels      11103946 ns     11100598 ns           63

*/
