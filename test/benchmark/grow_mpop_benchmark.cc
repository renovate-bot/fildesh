#include <cstdlib>
#include <vector>

#include <benchmark/benchmark.h>
#include <fildesh/fildesh.h>

#define THIS_BENCHMARK_RANGE \
  RangeMultiplier(2) \
  ->Range(1<<10, 1<<18)


static void BM_PushPop_FildeshA(benchmark::State& state) {
  for (auto _ : state) {
		unsigned char* at = NULL;
		size_t count = 0;
		Fildesh_lgsize allocated_lgcount = 0;
		for (int i = 0; i < state.range(0); ++i) {
			*static_cast<unsigned char*>(
          grow_FildeshA_((void**)(&at), &count, &allocated_lgcount, 1, 1))
        = static_cast<unsigned char>(i & 0xFF);
		}
		for (int i = 0; i < state.range(0); ++i) {
      assert(at);
      benchmark::DoNotOptimize(at[count-1]);
      mpop_FildeshA_((void**)&at, &count, &allocated_lgcount, 1, 1);
		}
    if (at) {free(at);}
  }
}
BENCHMARK(BM_PushPop_FildeshA)->THIS_BENCHMARK_RANGE;


static void BM_PushPop_FildeshAT(benchmark::State& state) {
  for (auto _ : state) {
    DECLARE_DEFAULT_FildeshAT(unsigned char, v);
		for (int i = 0; i < state.range(0); ++i) {
      push_FildeshAT(v, static_cast<unsigned char>(i & 0xFF));
		}
		for (int i = 0; i < state.range(0); ++i) {
      assert(*v);
      benchmark::DoNotOptimize(last_FildeshAT(v));
      mpop_FildeshAT(v, 1);
		}
    close_FildeshAT(v);
  }
}
BENCHMARK(BM_PushPop_FildeshAT)->THIS_BENCHMARK_RANGE;


static void BM_PushPop_StdVector(benchmark::State& state) {
  for (auto _ : state) {
    std::vector<unsigned char> at;
		for (int i = 0; i < state.range(0); ++i) {
      at.push_back(static_cast<unsigned char>(i & 0xFF));
		}
		for (int i = 0; i < state.range(0); ++i) {
      benchmark::DoNotOptimize(at[at.size()-1]);
      at.pop_back();
		}
  }
}
BENCHMARK(BM_PushPop_StdVector)->THIS_BENCHMARK_RANGE;


BENCHMARK_MAIN();
