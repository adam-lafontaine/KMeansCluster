#pragma once

#include "cluster.hpp"

#include <cmath>
#include <cassert>
#include <cstddef>

namespace cluster
{
	//======= CONSTANTS ========================

	constexpr size_t CLUSTER_ATTEMPTS = 30;
	constexpr size_t CLUSTER_ITERATIONS = 30;


	//======= DATA FUNCTIONS =======================


	// define how data type returns a value
	// for creating clusters from data
	constexpr value_t data_to_value(data_t const& data)
	{
		return static_cast<value_t>(data);
	}


	template<typename LHS_t, typename RHS_t>
	constexpr double distance_squared(LHS_t lhs, RHS_t rhs)
	{
		constexpr auto square = [](auto val) { return val * val; };

		return square(static_cast<double>(lhs) - static_cast<double>(rhs));
	}


	// calculates the average squared difference
	template<typename T>
	double list_distance(std::vector<T> const& lhs, std::vector<T> const& rhs)
	{
		double sum = 0;
		//auto size = std::min(lhs.size(), rhs.size());
		const auto size = lhs.size();

		for (size_t i = 0; i < size; ++i)
			sum += distance_squared(lhs[i], rhs[i]);

		return sum / size;
	}	


	//====== INITIALIZE DATA ==================

	// define how to initialize values based on type of value_row_t
	// used for initializing centroids
	inline
	value_row_t make_value_row(size_t capacity)
	{
		std::vector<value_t> row(capacity);		

		return row;
	}
	

	inline
	value_row_list_t make_value_row_list(size_t list_capacity, size_t row_capacity)
	{
		std::vector<value_row_t> list(list_capacity, make_value_row(row_capacity));
		return list;
	}
}