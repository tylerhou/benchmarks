#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <random>
#include <string>

#include "benchmark/benchmark.h"

static constexpr std::string_view kVowels = "aeiouAEIOU";

static bool HasVowelLoopConstexpr(std::string_view haystack) {
  for (char v : kVowels) {
    for (int i = 0; i < haystack.size(); ++i) {
      if (haystack[i] == v) {
        return true;
      }
    }
  }
  return false;
}

static bool HasVowelLoop(std::string_view vowels, std::string_view haystack) {
  for (char v : vowels) {
    for (int i = 0; i < haystack.size(); ++i) {
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

static void BM_HasVowelLoopConstexpr_ShortWithVowels(benchmark::State& state) {
  auto strs = ShortWithVowels();
  for (auto _ : state) {
    for (const std::string& s : strs)
      benchmark::DoNotOptimize(HasVowelLoopConstexpr(s));
  }
}

static void BM_HasVowelLoopConstexpr_ShortNoVowels(benchmark::State& state) {
  auto strs = ShortNoVowels();
  for (auto _ : state) {
    for (const std::string& s : strs)
      benchmark::DoNotOptimize(HasVowelLoopConstexpr(s));
  }
}
static void BM_HasVowelLoopConstexpr_LongWithVowels(benchmark::State& state) {
  auto strs = LongWithVowels();
  for (auto _ : state) {
    for (const std::string& s : strs)
      benchmark::DoNotOptimize(HasVowelLoopConstexpr(s));
  }
}
static void BM_HasVowelLoopConstexpr_LongNoVowels(benchmark::State& state) {
  auto strs = LongNoVowels();
  for (auto _ : state) {
    for (const std::string& s : strs)
      benchmark::DoNotOptimize(HasVowelLoopConstexpr(s));
  }
}

static void BM_HasVowelLoop_ShortWithVowels(benchmark::State& state) {
  auto strs = ShortWithVowels();
  for (auto _ : state) {
    for (const std::string& s : strs)
      benchmark::DoNotOptimize(HasVowelLoop(kVowels, s));
  }
}

static void BM_HasVowelLoop_ShortNoVowels(benchmark::State& state) {
  auto strs = ShortNoVowels();
  for (auto _ : state) {
    for (const std::string& s : strs)
      benchmark::DoNotOptimize(HasVowelLoop(kVowels, s));
  }
}
static void BM_HasVowelLoop_LongWithVowels(benchmark::State& state) {
  auto strs = LongWithVowels();
  for (auto _ : state) {
    for (const std::string& s : strs)
      benchmark::DoNotOptimize(HasVowelLoop(kVowels, s));
  }
}
static void BM_HasVowelLoop_LongNoVowels(benchmark::State& state) {
  auto strs = LongNoVowels();
  for (auto _ : state) {
    for (const std::string& s : strs)
      benchmark::DoNotOptimize(HasVowelLoop(kVowels, s));
  }
}

static void BM_HasVowelRegex_ShortWithVowels(benchmark::State& state) {
  auto strs = ShortWithVowels();
  for (auto _ : state) {
    for (const std::string& s : strs)
      benchmark::DoNotOptimize(HasVowelRegex(s));
  }
}

static void BM_HasVowelRegex_ShortNoVowels(benchmark::State& state) {
  auto strs = ShortNoVowels();
  for (auto _ : state) {
    for (const std::string& s : strs)
      benchmark::DoNotOptimize(HasVowelRegex(s));
  }
}
static void BM_HasVowelRegex_LongWithVowels(benchmark::State& state) {
  auto strs = LongWithVowels();
  for (auto _ : state) {
    for (const std::string& s : strs)
      benchmark::DoNotOptimize(HasVowelRegex(s));
  }
}
static void BM_HasVowelRegex_LongNoVowels(benchmark::State& state) {
  auto strs = LongNoVowels();
  for (auto _ : state) {
    for (const std::string& s : strs)
      benchmark::DoNotOptimize(HasVowelRegex(s));
  }
}

static void BM_HasVowelRegexEarlyReturn_ShortWithVowels(
    benchmark::State& state) {
  auto strs = ShortWithVowels();
  for (auto _ : state) {
    for (const std::string& s : strs)
      benchmark::DoNotOptimize(HasVowelRegexEarlyReturn(s));
  }
}

static void BM_HasVowelRegexEarlyReturn_ShortNoVowels(benchmark::State& state) {
  auto strs = ShortNoVowels();
  for (auto _ : state) {
    for (const std::string& s : strs)
      benchmark::DoNotOptimize(HasVowelRegexEarlyReturn(s));
  }
}
static void BM_HasVowelRegexEarlyReturn_LongWithVowels(
    benchmark::State& state) {
  auto strs = LongWithVowels();
  for (auto _ : state) {
    for (const std::string& s : strs)
      benchmark::DoNotOptimize(HasVowelRegexEarlyReturn(s));
  }
}
static void BM_HasVowelRegexEarlyReturn_LongNoVowels(benchmark::State& state) {
  auto strs = LongNoVowels();
  for (auto _ : state) {
    for (const std::string& s : strs)
      benchmark::DoNotOptimize(HasVowelRegexEarlyReturn(s));
  }
}

BENCHMARK(BM_HasVowelLoopConstexpr_ShortWithVowels);
BENCHMARK(BM_HasVowelLoop_ShortWithVowels);
BENCHMARK(BM_HasVowelRegex_ShortWithVowels);
BENCHMARK(BM_HasVowelRegexEarlyReturn_ShortWithVowels);

BENCHMARK(BM_HasVowelLoopConstexpr_ShortNoVowels);
BENCHMARK(BM_HasVowelLoop_ShortNoVowels);
BENCHMARK(BM_HasVowelRegex_ShortNoVowels);
BENCHMARK(BM_HasVowelRegexEarlyReturn_ShortNoVowels);

BENCHMARK(BM_HasVowelLoopConstexpr_LongWithVowels);
BENCHMARK(BM_HasVowelLoop_LongWithVowels);
BENCHMARK(BM_HasVowelRegex_LongWithVowels);
BENCHMARK(BM_HasVowelRegexEarlyReturn_LongWithVowels);

BENCHMARK(BM_HasVowelLoopConstexpr_LongNoVowels);
BENCHMARK(BM_HasVowelLoop_LongNoVowels);
BENCHMARK(BM_HasVowelRegex_LongNoVowels);
BENCHMARK(BM_HasVowelRegexEarlyReturn_LongNoVowels);

/* Results on my M1 Mac:

tylerhou@ ~/code/benchmarks main*
❯ bazel run -c opt :vowels-benchmark_test
Starting local Bazel server and connecting to it...
INFO: Analyzed target //:vowels-benchmark_test (77 packages loaded, 396 targets configured).
INFO: Found 1 target...
Target //:vowels-benchmark_test up-to-date:
  bazel-bin/vowels-benchmark_test
INFO: Elapsed time: 11.254s, Critical Path: 2.26s
INFO: 39 processes: 16 internal, 23 processwrapper-sandbox.
INFO: Build completed successfully, 39 total actions
INFO: Running command line: external/bazel_tools/tools/test/test-setup.sh ./vowels-benchmark_test
exec ${PAGER:-/usr/bin/less} "$0" || exit 1
Executing tests from //:vowels-benchmark_test
-----------------------------------------------------------------------------
2025-06-14T11:53:48+00:00
Running /home/tylerhou/.cache/bazel/_bazel_tylerhou/09f0f2c0c9650554e1952e274ae6605c/execroot/_main/bazel-out/aarch64-opt/bin/vowels-benchmark_test.runfiles/_main/vowels-benchmark_test
Run on (4 X 48 MHz CPU s)
CPU Caches:
  L1 Data 128 KiB (x4)
  L1 Instruction 192 KiB (x4)
  L2 Unified 12288 KiB (x4)
Load Average: 0.85, 0.30, 0.26
--------------------------------------------------------------------------------------
Benchmark                                            Time             CPU   Iterations
--------------------------------------------------------------------------------------
BM_HasVowelLoopConstexpr_ShortWithVowels         21351 ns        21349 ns        32294
BM_HasVowelLoop_ShortWithVowels                  20954 ns        20953 ns        33480
BM_HasVowelRegex_ShortWithVowels                  8598 ns         8594 ns        76750
BM_HasVowelRegexEarlyReturn_ShortWithVowels       4698 ns         4697 ns       147919
BM_HasVowelLoopConstexpr_ShortNoVowels           49714 ns        49709 ns        14102
BM_HasVowelLoop_ShortNoVowels                    49373 ns        49361 ns        14149
BM_HasVowelRegex_ShortNoVowels                    8957 ns         8953 ns        73880
BM_HasVowelRegexEarlyReturn_ShortNoVowels        16544 ns        16542 ns        39720
BM_HasVowelLoopConstexpr_LongWithVowels          31675 ns        31662 ns        22046
BM_HasVowelLoop_LongWithVowels                   30906 ns        30894 ns        22785
BM_HasVowelRegex_LongWithVowels               11009363 ns     11003624 ns           64
BM_HasVowelRegexEarlyReturn_LongWithVowels        6539 ns         6536 ns       107622
BM_HasVowelLoopConstexpr_LongNoVowels         16600642 ns     16595016 ns           42
BM_HasVowelLoop_LongNoVowels                  16592748 ns     16587245 ns           41
BM_HasVowelRegex_LongNoVowels                 11003512 ns     10997950 ns           64
BM_HasVowelRegexEarlyReturn_LongNoVowels      11094402 ns     11089638 ns           63

*/
