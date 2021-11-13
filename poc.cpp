#include "poc.hpp"

#include <vector>

namespace poc
{
	size_t GetSum(const std::vector<float>& data)
	{
		size_t sum{ 0 };
		for (const auto d : data)
			sum += static_cast<size_t>(d + 0.5f);

		return sum;
	}

	size_t NoTemplate(const std::vector<float>& data, size_t add)
	{
		return GetSum(data) + add;
	}
} // namespace poc
