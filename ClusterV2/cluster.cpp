#include "cluster_config.hpp"

#include <cstdlib>
#include <algorithm>
#include <random>
#include <iterator>
#include <iostream>
#include <functional>

namespace cluster
{
	//======= DATA FUNCTIONS =======================


	template<typename LHS_t, typename RHS_t>
	double distance_squared(LHS_t lhs, RHS_t rhs)
	{
		constexpr auto square = [](auto val) { return val * val; };

		return square(static_cast<double>(lhs) - static_cast<double>(rhs));
	}


	// calculates the average spared difference
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
	value_row_t make_value_row(size_t capacity)
	{
		std::vector<value_t> row(capacity);

		return row;
	}

	value_row_list_t make_value_row_list(size_t list_capacity, size_t row_capacity)
	{
		std::vector<value_row_t> list(list_capacity, make_value_row(row_capacity));
		return list;
	}


	//======= TYPES ===================

	typedef struct ClusterCount // used for tracking the number of times a given result is found
	{
		cluster_result_t result;
		unsigned count;

	} cluster_count_t;

	using closest_t = std::function<distance_result_t(data_row_t const& data, value_row_list_t const& value_list)>;

	using cluster_once_t = std::function<cluster_result_t(data_row_list_t const& x_list, size_t num_clusters)>;

	
	//======= HELPERS ====================

	static size_t max_value(index_list_t const& list)
	{
		return *std::max_element(list.begin(), list.end());
	}


	// convert a list of data_row_t to value_row_t
	static value_row_list_t to_value_row_list(data_row_list_t const& data_row_list, to_value_funct_t const& converter)
	{
		auto list = make_value_row_list(data_row_list.size(), data_row_list[0].size());
		for (size_t i = 0; i < data_row_list.size(); ++i)
		{
			auto data_row = data_row_list[i];
			for (size_t j = 0; j < data_row.size(); ++j)
				list[i][j] = converter(data_row[j]);
		}

		return list;
	}


	// selects random data to be used as centroids
	static value_row_list_t get_random_centroids(data_row_list_t const& x_list, size_t num_clusters, to_value_funct_t const& converter)
	{
		// C++ 17 std::sample

		data_row_list_t samples;
		samples.reserve(num_clusters);

		std::sample(x_list.begin(), x_list.end(), std::back_inserter(samples),
			num_clusters, std::mt19937{ std::random_device{}() });

		return to_value_row_list(samples, converter);
	}
		

	// assigns a cluster index to each data point
	static cluster_result_t assign_clusters(data_row_list_t const& x_list, value_row_list_t& centroids, closest_t const& closest)
	{
		index_list_t x_clusters;
		x_clusters.reserve(x_list.size());

		double total_distance = 0;

		for (auto const& x_data : x_list)
		{
			auto c = closest(x_data, centroids);

			x_clusters.push_back(c.index);
			total_distance += c.distance;
		}

		cluster_result_t res = { std::move(x_clusters), std::move(centroids), total_distance / x_list.size() };
		return res;
	}


	// finds new centroids based on the averages of data clustered together
	static value_row_list_t calc_centroids(data_row_list_t const& x_list, index_list_t const& x_clusters, size_t num_clusters, to_value_funct_t const& converter)
	{
		const auto data_size = x_list[0].size();
		auto values = make_value_row_list(num_clusters, data_size);		
		
		std::vector<unsigned> counts(num_clusters, 0);

		for (size_t i = 0; i < x_list.size(); ++i)
		{
			const auto cluster_index = x_clusters[i];
			++counts[cluster_index];

			for (size_t d = 0; d < data_size; ++d)
				values[cluster_index][d] += converter(x_list[i][d]); // totals for each cluster
		}

		for (size_t k = 0; k < num_clusters; ++k)
		{
			for (size_t d = 0; d < data_size; ++d)
				values[k][d] = values[k][d] / counts[k]; // convert to average
		}

		return values;
	}


	// re-label cluster assignments so that they are consistent accross iterations
	static void relabel_clusters(cluster_result_t& result, size_t num_clusters)
	{
		std::vector<uint8_t> flags(num_clusters, 0); // tracks if cluster index has been mapped
		std::vector<size_t> map(num_clusters, 0);    // maps old cluster index to new cluster index

		const auto all_flagged = [&]()
		{
			for (auto const flag : flags)
			{
				if (!flag)
					return false;
			}

			return true;
		};

		size_t i = 0;
		size_t label = 0;

		for (; label < num_clusters && i < result.x_clusters.size() && !all_flagged(); ++i)
		{
			size_t c = result.x_clusters[i];
			if (flags[c])
				continue;

			map[c] = label;
			flags[c] = 1;
			++label;
		}

		// re-label cluster assignments
		for (i = 0; i < result.x_clusters.size(); ++i)
		{
			size_t c = result.x_clusters[i];
			result.x_clusters[i] = map[c];
		}
	}


	//======= CLUSTERING ALGORITHMS ==========================
	

	// returns the result with the smallest distance
	static cluster_result_t cluster_min_distance(data_row_list_t const& x_list, size_t num_clusters, cluster_once_t const& cluster_once)
	{
		auto result = cluster_once(x_list, num_clusters);
		auto min = result;

		for (size_t i = 0; i < CLUSTER_ATTEMPTS; ++i)
		{
			result = cluster_once(x_list, num_clusters);
			if (result.average_distance < min.average_distance)
				min = std::move(result);
		}

		return min;
	}
	
	/*

	// returns the most popular result
	// stops when the same result has been found for more than half of the attempts
	static cluster_result_t cluster_max_count(data_row_list_t const& x_list, size_t num_clusters, cluster_once_t const& cluster_once)
	{
		std::vector<cluster_count_t> counts;
		counts.reserve(CLUSTER_ATTEMPTS);

		auto result = cluster_once(x_list, num_clusters);
		counts.push_back({ std::move(result), 1 });

		for (size_t i = 0; i < CLUSTER_ATTEMPTS; ++i)
		{
			result = cluster_once(x_list, num_clusters);

			bool add_clusters = true;
			for (auto& c : counts)
			{
				if (list_distance(result.x_clusters, c.result.x_clusters) != 0)
					continue;

				++c.count;
				if (c.count > CLUSTER_ATTEMPTS / 2)
					return c.result;

				add_clusters = false;
				break;
			}

			if (add_clusters)
			{
				counts.push_back({ std::move(result), 1 });
			}
		}

		auto constexpr comp = [](cluster_count_t const& lhs, cluster_count_t const& rhs) { return lhs.count < rhs.count; };
		auto const best = *std::max_element(counts.begin(), counts.end(), comp);

		return best.result;
	}

	*/


	//======= CLASS METHODS ==============================

	distance_result_t Cluster::closest(data_row_t const& data, value_row_list_t const& value_list) const
	{
		distance_result_t res = { 0, m_distance(data, value_list[0]) };

		for (size_t i = 1; i < value_list.size(); ++i)
		{
			auto dist = m_distance(data, value_list[i]);
			if (dist < res.distance)
			{
				res.distance = dist;
				res.index = i;
			}
		}

		return res;
	}



	size_t Cluster::find_centroid(data_row_t const& data, value_row_list_t const& centroids) const
	{
		auto result = closest(data, centroids);

		return result.index;
	}


	cluster_result_t Cluster::cluster_once(data_row_list_t const& x_list, size_t num_clusters) const
	{
		auto const closest_f = [&](data_row_t const& data, value_row_list_t const& value_list)
		{
			return closest(data, value_list);
		};

		auto centroids = get_random_centroids(x_list, num_clusters, m_to_value); // start with random data as centroids

		auto result = assign_clusters(x_list, centroids, closest_f);
		relabel_clusters(result, num_clusters);

		for (size_t i = 0; i < CLUSTER_ITERATIONS; ++i)
		{
			centroids = calc_centroids(x_list, result.x_clusters, num_clusters, m_to_value);
			auto res_try = assign_clusters(x_list, centroids, closest_f);

			if (max_value(res_try.x_clusters) < num_clusters - 1)
				continue;

			auto res_old = std::move(result);
			result = std::move(res_try);
			relabel_clusters(result, num_clusters);

			if (list_distance(res_old.x_clusters, result.x_clusters) == 0)
				return result;
		}

		return result;
	}


	cluster_result_t Cluster::cluster_data(data_row_list_t const& x_list, size_t num_clusters) const
	{
		// wrap member function in a lambda to pass it to algorithm
		auto const cluster_once_f = [&](data_row_list_t const& x_list, size_t num_clusters)
		{
			return cluster_once(x_list, num_clusters);
		};

		return cluster_min_distance(x_list, num_clusters, cluster_once_f);
	}
}

