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

static constexpr std::string_view kCharsWithVowels =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static constexpr std::string_view kCharsNoVowels = "0123456789"
                                                   "bcdfghjklmnpqrstvwxyz"
                                                   "BCDFGHJKLMNPQRSTVWXYZ";

static constexpr int kShortNumStrings = 1'000;
static constexpr int kLongNumStrings = 1'000;

static constexpr auto ShortStringDistribution(std::mt19937 &rng) {
  return [&]() {
    std::binomial_distribution<> d(15, 0.5);
    return d(rng) + 5;
  };
}

static constexpr auto LongStringDistribution(std::mt19937 &rng) {
  return [&]() {
    std::binomial_distribution<> d(10000, 0.5);
    return d(rng);
  };
}

template <typename T>
static std::unique_ptr<std::vector<std::string>>
MakeStrings(std::mt19937 &rng, std::string_view characters, int num_strings,
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

static std::vector<std::string> &ShortWithVowels() {
  static std::vector<std::string> *strs = nullptr;
  if (strs == nullptr) {
    std::random_device dev;
    std::mt19937 rng(dev());
    strs = MakeStrings(rng, kCharsWithVowels, /*num_strings=*/kShortNumStrings,
                       ShortStringDistribution(rng))
               .release();
  }

  return *strs;
}

static std::vector<std::string> &ShortNoVowels() {
  static std::vector<std::string> *strs = nullptr;
  if (strs == nullptr) {
    std::random_device dev;
    std::mt19937 rng(dev());
    strs = MakeStrings(rng, kCharsNoVowels, /*num_strings=*/kShortNumStrings,
                       ShortStringDistribution(rng))
               .release();
  }

  return *strs;
}
static std::vector<std::string> &LongWithVowels() {
  static std::vector<std::string> *strs = nullptr;
  if (strs == nullptr) {
    std::random_device dev;
    std::mt19937 rng(dev());
    strs = MakeStrings(rng, kCharsWithVowels, /*num_strings=*/kLongNumStrings,
                       LongStringDistribution(rng))
               .release();
  }

  return *strs;
}

static std::vector<std::string> &LongNoVowels() {
  static std::vector<std::string> *strs = nullptr;
  if (strs == nullptr) {
    std::random_device dev;
    std::mt19937 rng(dev());
    strs = MakeStrings(rng, kCharsNoVowels, /*num_strings=*/kLongNumStrings,
                       LongStringDistribution(rng))
               .release();
  }

  return *strs;
}

static void BM_HasVowelLoopConstexpr_ShortWithVowels(benchmark::State &state) {
  auto strs = ShortWithVowels();
  for (auto _ : state) {
    for (const std::string &s : strs)
      benchmark::DoNotOptimize(HasVowelLoopConstexpr(s));
  }
}

static void BM_HasVowelLoopConstexpr_ShortNoVowels(benchmark::State &state) {
  auto strs = ShortNoVowels();
  for (auto _ : state) {
    for (const std::string &s : strs)
      benchmark::DoNotOptimize(HasVowelLoopConstexpr(s));
  }
}
static void BM_HasVowelLoopConstexpr_LongWithVowels(benchmark::State &state) {
  auto strs = LongWithVowels();
  for (auto _ : state) {
    for (const std::string &s : strs)
      benchmark::DoNotOptimize(HasVowelLoopConstexpr(s));
  }
}
static void BM_HasVowelLoopConstexpr_LongNoVowels(benchmark::State &state) {
  auto strs = LongNoVowels();
  for (auto _ : state) {
    for (const std::string &s : strs)
      benchmark::DoNotOptimize(HasVowelLoopConstexpr(s));
  }
}

static void BM_HasVowelLoop_ShortWithVowels(benchmark::State &state) {
  auto strs = ShortWithVowels();
  for (auto _ : state) {
    for (const std::string &s : strs)
      benchmark::DoNotOptimize(HasVowelLoop(kVowels, s));
  }
}

static void BM_HasVowelLoop_ShortNoVowels(benchmark::State &state) {
  auto strs = ShortNoVowels();
  for (auto _ : state) {
    for (const std::string &s : strs)
      benchmark::DoNotOptimize(HasVowelLoop(kVowels, s));
  }
}
static void BM_HasVowelLoop_LongWithVowels(benchmark::State &state) {
  auto strs = LongWithVowels();
  for (auto _ : state) {
    for (const std::string &s : strs)
      benchmark::DoNotOptimize(HasVowelLoop(kVowels, s));
  }
}
static void BM_HasVowelLoop_LongNoVowels(benchmark::State &state) {
  auto strs = LongNoVowels();
  for (auto _ : state) {
    for (const std::string &s : strs)
      benchmark::DoNotOptimize(HasVowelLoop(kVowels, s));
  }
}

static void BM_HasVowelRegex_ShortWithVowels(benchmark::State &state) {
  auto strs = ShortWithVowels();
  for (auto _ : state) {
    for (const std::string &s : strs)
      benchmark::DoNotOptimize(HasVowelRegex(s));
  }
}

static void BM_HasVowelRegex_ShortNoVowels(benchmark::State &state) {
  auto strs = ShortNoVowels();
  for (auto _ : state) {
    for (const std::string &s : strs)
      benchmark::DoNotOptimize(HasVowelRegex(s));
  }
}
static void BM_HasVowelRegex_LongWithVowels(benchmark::State &state) {
  auto strs = LongWithVowels();
  for (auto _ : state) {
    for (const std::string &s : strs)
      benchmark::DoNotOptimize(HasVowelRegex(s));
  }
}
static void BM_HasVowelRegex_LongNoVowels(benchmark::State &state) {
  auto strs = LongNoVowels();
  for (auto _ : state) {
    for (const std::string &s : strs)
      benchmark::DoNotOptimize(HasVowelRegex(s));
  }
}

BENCHMARK(BM_HasVowelLoopConstexpr_ShortWithVowels);
BENCHMARK(BM_HasVowelLoop_ShortWithVowels);
BENCHMARK(BM_HasVowelRegex_ShortWithVowels);

BENCHMARK(BM_HasVowelLoopConstexpr_ShortNoVowels);
BENCHMARK(BM_HasVowelLoop_ShortNoVowels);
BENCHMARK(BM_HasVowelRegex_ShortNoVowels);

BENCHMARK(BM_HasVowelLoopConstexpr_LongWithVowels);
BENCHMARK(BM_HasVowelLoop_LongWithVowels);
BENCHMARK(BM_HasVowelRegex_LongWithVowels);

BENCHMARK(BM_HasVowelLoopConstexpr_LongNoVowels);
BENCHMARK(BM_HasVowelLoop_LongNoVowels);
BENCHMARK(BM_HasVowelRegex_LongNoVowels);

/* Results on my M1 Mac:

tylerhou@ ~/code/cpp-template main* 13s
❯ bazel run -c opt :vowels-benchmark_test
INFO: Analyzed target //:vowels-benchmark_test (0 packages loaded, 0 targets
configured). INFO: Found 1 target... Target //:vowels-benchmark_test up-to-date:
  bazel-bin/vowels-benchmark_test
INFO: Elapsed time: 1.149s, Critical Path: 1.05s
INFO: 3 processes: 1 internal, 2 processwrapper-sandbox.
INFO: Build completed successfully, 3 total actions
INFO: Running command line: external/bazel_tools/tools/test/test-setup.sh
./vowels-benchmark_test exec ${PAGER:-/usr/bin/less} "$0" || exit 1 Executing
tests from //:vowels-benchmark_test
-----------------------------------------------------------------------------
2025-06-14T11:38:11+00:00
Running
/home/tylerhou/.cache/bazel/_bazel_tylerhou/98d00b3bb5932a341771f3056b492536/execroot/_main/bazel-out/aarch64-opt/bin/vowels-benchmark_test.runfiles/_main/vowels-benchmark_test
Run on (4 X 48 MHz CPU s)
CPU Caches:
  L1 Data 128 KiB (x4)
  L1 Instruction 192 KiB (x4)
  L2 Unified 12288 KiB (x4)
Load Average: 0.43, 0.44, 0.38
-----------------------------------------------------------------------------------
Benchmark                                         Time             CPU
Iterations
-----------------------------------------------------------------------------------
BM_HasVowelLoopConstexpr_ShortWithVowels      21399 ns        21400 ns 32364
BM_HasVowelLoop_ShortWithVowels               21940 ns        21941 ns 32027
BM_HasVowelRegex_ShortWithVowels               8474 ns         8466 ns 79481
BM_HasVowelLoopConstexpr_ShortNoVowels        49166 ns        49169 ns 14286
BM_HasVowelLoop_ShortNoVowels                 49741 ns        49741 ns 14065
BM_HasVowelRegex_ShortNoVowels                 8685 ns         8685 ns 80750
BM_HasVowelLoopConstexpr_LongWithVowels       33160 ns        33148 ns 20964
BM_HasVowelLoop_LongWithVowels                33149 ns        33138 ns 21155
BM_HasVowelRegex_LongWithVowels            10916078 ns     10911679 ns 64
BM_HasVowelLoopConstexpr_LongNoVowels      16588231 ns     16583037 ns 42
BM_HasVowelLoop_LongNoVowels               16607384 ns     16600469 ns 42
BM_HasVowelRegex_LongNoVowels              11048400 ns     11043750 ns 64

*/
