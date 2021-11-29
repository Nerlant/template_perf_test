#pragma once

#include <type_traits>
#include <vector>

namespace poc
{
	size_t GetSum(const std::vector<float>& data);

	template<typename ContainerType, typename GetSumFn, typename SumType = std::invoke_result_t<GetSumFn, ContainerType>>
	size_t Template(const ContainerType& data, SumType add, GetSumFn get_sum)
	{
		static_assert(std::is_convertible_v<SumType, size_t>);

		return get_sum(data) + add;
	}

	size_t NoTemplate(const std::vector<float>& data, size_t add);
} // namespace poc
