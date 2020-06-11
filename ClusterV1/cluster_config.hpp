#pragma once
#include "cluster.hpp"

#include <cmath>

namespace cluster
{
	//======= CONSTANTS ========================

	constexpr size_t CLUSTER_ATTEMPTS = 30;
	constexpr size_t CLUSTER_ITERATIONS = 30;


	//======= DATA FUNCTIONS =======================


	// define how data type returns a value
	constexpr value_t data_to_value(data_t const& data)
	{
		return static_cast<value_t>(data);
		//return data;
	}


	// define how a value_t gets converted to data_t
	inline data_t value_to_data(value_t const& value)
	{
		return static_cast<data_t>(std::round(value));
	}

	template<typename LHS_t, typename RHS_t> 
	constexpr double distance_squared(LHS_t lhs, RHS_t rhs)
	{
		constexpr auto square = [](auto val) { return val * val; };

		return square(static_cast<double>(lhs) - static_cast<double>(rhs));
	}

	template<typename T>
	double list_distance(std::vector<T> const& lhs, std::vector<T> const& rhs)
	{
		double sum = 0;
		//auto size = std::min(lhs.size(), rhs.size());
		auto size = lhs.size();

		for (size_t i = 0; i < size; ++i)
			sum += distance_squared(lhs[i], rhs[i]);

		return sum;
	}

	// define distance for data_row_t
	constexpr double data_distance(data_row_t const& x1, data_row_t const& x2, size_t data_size)
	{
		double sum = 0;
		for (size_t i = 0; i < data_size; ++i)
			sum += distance_squared(data_to_value(x1[i]), data_to_value(x2[i]));

		return sum;
	}

	double data_distance(data_row_t const& x1, data_row_t const& x2)
	{
		double sum = 0;
		for (size_t i = 0; i < x1.size(); ++i)
			sum += distance_squared(data_to_value(x1[i]), data_to_value(x2[i]));

		return sum;
	}

	constexpr double value_distance(data_row_t const& data, value_row_t const& val, size_t data_size)
	{
		double sum = 0;
		for (size_t i = 0; i < data_size; ++i)
			sum += distance_squared(data_to_value(data[i]), val[i]);

		return sum;
	}

	double value_distance(data_row_t const& data, value_row_t const& val)
	{
		double sum = 0;
		for (size_t i = 0; i < data.size(); ++i)
			sum += distance_squared(data_to_value(data[i]), val[i]);

		return sum;
	}


	//====== INITIALIZE DATA ==================

	// define how to initialize values based on type of value_row_t
	inline
	value_row_t make_value_row(size_t capacity)
	{
		/*std::string row(capacity, 'x');
		return row;*/

		/*std::array<value_t, 100> row = { 0 };
		return row;*/

		std::vector<value_t> row(capacity);
		

		return row;
	}

	inline
	value_row_list_t make_value_row_list(size_t list_capacity, size_t row_capacity)
	{
		std::vector<value_row_t> list(list_capacity, make_value_row(row_capacity));
		return list;
	}

	
	inline
	data_row_t make_data_row(size_t capacity)
	{
		std::string row(capacity, 'x');
		return row;

		/*std::array<value_t, 100> row = { 0 };
		return row;*/

		/*std::vector<value_t> row;
		if (capacity)
			row.reserve(capacity);*/
	}


	inline
	data_row_list_t make_data_row_list(size_t list_capacity, size_t row_capacity)
	{
		std::vector<data_row_t> list(list_capacity, make_data_row(row_capacity));
		return list;
	}
}