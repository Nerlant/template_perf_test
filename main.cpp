#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <vector>

#include "poc.hpp"

// Prefer high_resolution_clock, but only if it's steady...
template<bool HighResIsSteady = std::chrono::high_resolution_clock::is_steady>
struct SteadyClock {
    using type = std::chrono::high_resolution_clock;
};

// ...otherwise use steady_clock.
template<>
struct SteadyClock<false> {
    using type = std::chrono::steady_clock;
};

inline SteadyClock<>::type::time_point benchmark_now() {
    return SteadyClock<>::type::now();
}

inline double benchmark_duration_seconds(
    SteadyClock<>::type::time_point start,
    SteadyClock<>::type::time_point end) {
    return std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
}

inline double benchmark(uint64_t samples, uint64_t iterations, const std::function<void()>& op) {
    double best = std::numeric_limits<double>::infinity();
    for (uint64_t i = 0; i < samples; i++) {
        auto start = benchmark_now();
        for (uint64_t j = 0; j < iterations; j++) {
            op();
        }
        auto end = benchmark_now();
        double elapsed_seconds = benchmark_duration_seconds(start, end);
        best = std::min(best, elapsed_seconds);
    }
    return best / iterations;
}

struct BenchmarkConfig 
{
    // Attempt to use this much time (in seconds) for the meaningful samples
    // taken; initial iterations will be done to find an iterations-per-sample
    // count that puts the total runtime in this ballpark.
    double min_time{ 0.1 };

    // Set an absolute upper time limit. Defaults to min_time * 4.
    double max_time{ 0.1 * 4 };

    // Terminate when the relative difference between the best runtime
    // seen and the third-best runtime seen is no more than
    // this. Controls accuracy. The closer to zero this gets the more
    // reliable the answer, but the longer it may take to run.
    double accuracy{ 0.03 };
};

struct BenchmarkResult 
{
    // Best elapsed wall-clock time per iteration (seconds).
    double wall_time;

    // Number of samples used for measurement.
    // (There might be additional samples taken that are not used
    // for measurement.)
    uint64_t samples;

    // Total number of iterations across all samples.
    // (There might be additional iterations taken that are not used
    // for measurement.)
    uint64_t iterations;

    // Measured accuracy between the best and third-best result.
    // Will be <= config.accuracy unless max_time is exceeded.
    double accuracy;

    operator double() const {
        return wall_time;
    }
};

inline BenchmarkResult benchmark(const std::function<void()>& op, const BenchmarkConfig& config = {}) {
    BenchmarkResult result{ 0, 0, 0 };

    const double min_time = std::max(10 * 1e-6, config.min_time);
    const double max_time = std::max(config.min_time, config.max_time);

    const double accuracy = 1.0 + std::min(std::max(0.001, config.accuracy), 0.1);

    // We will do (at least) kMinSamples samples; we will do additional
    // samples until the best the kMinSamples'th results are within the
    // accuracy tolerance (or we run out of iterations).
    constexpr int kMinSamples = 3;
    double times[kMinSamples + 1] = { 0 };

    double total_time = 0;
    uint64_t iters_per_sample = 1;
    for (;;) {
        result.samples = 0;
        result.iterations = 0;
        total_time = 0;
        for (int i = 0; i < kMinSamples; i++) {
            times[i] = benchmark(1, iters_per_sample, op);
            result.samples++;
            result.iterations += iters_per_sample;
            total_time += times[i] * iters_per_sample;
        }
        std::sort(times, times + kMinSamples);
        if (times[0] * iters_per_sample * kMinSamples >= min_time) {
            break;
        }
        // Use an estimate based on initial times to converge faster.
        double next_iters = std::max(min_time / std::max(times[0] * kMinSamples, 1e-9),
            iters_per_sample * 2.0);
        iters_per_sample = (uint64_t)(next_iters + 0.5);
    }

    // - Keep taking samples until we are accurate enough (even if we run over min_time).
    // - If we are already accurate enough but have time remaining, keep taking samples.
    // - No matter what, don't go over max_time; this is important, in case
    // we happen to get faster results for the first samples, then happen to transition
    // to throttled-down CPU state.
    while ((times[0] * accuracy < times[kMinSamples - 1] || total_time < min_time) &&
        total_time < max_time) {
        times[kMinSamples] = benchmark(1, iters_per_sample, op);
        result.samples++;
        result.iterations += iters_per_sample;
        total_time += times[kMinSamples] * iters_per_sample;
        std::sort(times, times + kMinSamples + 1);
    }
    result.wall_time = times[0];
    result.accuracy = (times[kMinSamples - 1] / times[0]) - 1.0;

    return result;
}

int main()
{
	// generate some random data
	std::vector<float> data(2000);
	std::srand(static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
	for (auto& d : data)
	{
		d = static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) / 1e10f);
	}
	
	std::cout << poc::NoTemplate(data, 1337) << '\n';
	std::cout << poc::Template(data, 1337, poc::GetSum) << '\n';

    const auto l1 = [&]() { poc::NoTemplate(data, 1337); };
    const auto r1 = benchmark(l1);
    std::cout << "\nTime for NoTemplate: " << r1.wall_time << "s\n";

    const auto l2 = [&]() { poc::Template(data, 1337, poc::GetSum); };
    const auto r2 = benchmark(l2);
    std::cout << "\nTime for Template: " << r2.wall_time << "s\n";

	return 0;
}
