#include "cluster_config.hpp"
#include "stopwatch.hpp"

#include <cstdlib>
#include <algorithm>
#include <random>
#include <iterator>
#include <iostream>

namespace cluster
{
	typedef struct DistanceResult {
		size_t index;
		double distance;

	} distance_result_t;

	typedef struct ClusterCount {

		cluster_result_t result;
		unsigned count;

	} cluster_count_t;



	



	distance_result_t closest(data_row_t const& data, value_row_list_t const& value_list)
	{
		distance_result_t res = { 0, value_distance(data, value_list[0]) };

		for (size_t i = 1; i < value_list.size(); ++i)
		{
			auto dist = value_distance(data, value_list[i]);
			if (dist < res.distance)
			{
				res.distance = dist;
				res.index = i;
			}
		}

		return res;
	}



	value_row_list_t calc_centroids(data_row_list_t const& x_list, index_list_t const& x_clusters, size_t num_clusters)
	{
		const auto data_size = x_list[0].size();
		auto values = make_value_row_list(num_clusters, data_size);
		
		
		std::vector<unsigned> counts(num_clusters, 0);

		for (size_t i = 0; i < x_list.size(); ++i)
		{
			const auto cluster_index = x_clusters[i];
			++counts[cluster_index];

			for (size_t d = 0; d < data_size; ++d)
				values[cluster_index][d] += data_to_value(x_list[i][d]); // totals for each cluster
		}

		for (size_t k = 0; k < num_clusters; ++k)
		{
			for (size_t d = 0; d < data_size; ++d)
				values[k][d] = values[k][d] / counts[k]; // convert to average
		}

		return values;
	}

	// re-label cluster assignments so that they are consistent accross iterations
	void relabel_clusters(cluster_result_t& result, size_t num_clusters)
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

	cluster_result_t assign_clusters(data_row_list_t const& x_list, value_row_list_t& centroids, size_t num_clusters)  // TODO: remove num_clusters
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



	value_row_list_t random_values(data_row_list_t const& x_list, size_t num_clusters)
	{
		// C++ 17 std::sample

		data_row_list_t samples;
		samples.reserve(num_clusters);

		std::sample(x_list.begin(), x_list.end(), std::back_inserter(samples),
			num_clusters, std::mt19937{ std::random_device{}() });

		return to_value_row_list(samples);
	}

	size_t max_value(index_list_t const& list)
	{
		return *std::max_element(list.begin(), list.end());
	}

	cluster_result_t cluster_once(data_row_list_t const& x_list, size_t num_clusters)
	{
		auto centroids = random_values(x_list, num_clusters);
		auto result = assign_clusters(x_list, centroids, num_clusters);
		relabel_clusters(result, num_clusters);

		for (size_t i = 0; i < CLUSTER_ITERATIONS; ++i)
		{
			centroids = calc_centroids(x_list, result.x_clusters, num_clusters);
			auto res_try = assign_clusters(x_list, centroids, num_clusters);

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


	// returns the result with the smallest distance
	cluster_result_t cluster_min_distance(data_row_list_t const& x_list, size_t num_clusters)
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


	// returns the most popular result
	// stops when the same result has been found for more than half of the attempts
	cluster_result_t cluster_max_count(data_row_list_t const& x_list, size_t num_clusters)
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


	// keeps increasing the number of clusters until the incremental improvement is small enough
	cluster_result_t cluster_unknown(data_row_list_t const& x_list, size_t min_clusters, size_t max_clusters)
	{
		auto const cluster_algorithm = cluster_max_count;

		// cannot compare, 3 will allways be better than 2
		if (max_clusters <= 3)
			return cluster_algorithm(x_list, max_clusters);

		constexpr double improve_tolerance = 0.1;
		auto last = cluster_algorithm(x_list, min_clusters - 2); std::cout << 2 << " : " << last.average_distance << "\n";
		auto next = cluster_algorithm(x_list, min_clusters - 1); std::cout << 3 << " : " << next.average_distance << "\n";

		auto last_improvement = last.average_distance - next.average_distance;

		for (size_t k = min_clusters; k <= max_clusters; ++k)
		{
			last = std::move(next);
			next = cluster_algorithm(x_list, k); std::cout << k << " : " << next.average_distance << "\n";
			auto improvement = last.average_distance - next.average_distance;

			if (improvement < improve_tolerance * next.average_distance)
				return last;

			last_improvement = improvement;
		}

		return next;
	}


	// for trying to find the best number of clusters
	void find_clusters(data_row_list_t const& x_list, size_t max_clusters)
	{
		auto const cluster_algorithm = cluster_max_count;
		const size_t min_clusters = 2;
		Stopwatch stop_watch;
		double last_time = 0;
		double last_dist = 0;

		for (size_t k = min_clusters; k <= max_clusters; ++k)
		{
			stop_watch.start();

			auto result = cluster_algorithm(x_list, k);
			auto time = stop_watch.get_time_sec();
			auto dist = result.average_distance;

			std::cout << k << " | "
				<< dist << " (" << (1 - dist / last_dist) << ")" << " | "
				<< time << "(" << (time - last_time) << ")" << "\n";

			last_time = time;
			last_dist = dist;
		}


	}


	// convert a list of value_row_t to data_row_t
	data_row_list_t to_data_row_list(value_row_list_t const& value_row_list)
	{
		auto list = make_data_row_list(value_row_list.size(), value_row_list[0].size());
		for (size_t i = 0; i < value_row_list.size(); ++i)
		{
			auto value_row = value_row_list[i];
			for (size_t j = 0; j < value_row.size(); ++j)
				list[i][j] = value_to_data(value_row[j]);
		}
			
		return list;
	}


	// convert a list of data_row_t to value_row_t
	value_row_list_t to_value_row_list(data_row_list_t const& data_row_list)
	{
		auto list = make_value_row_list(data_row_list.size(), data_row_list[0].size());
		for (size_t i = 0; i < data_row_list.size(); ++i)
		{
			auto data_row = data_row_list[i];
			for (size_t j = 0; j < data_row.size(); ++j)
				list[i][j] = data_to_value(data_row[j]);
		}

		return list;
	}


	// finds centroid closest to data
	value_row_t find_centroid(data_row_t const& data, value_row_list_t const& value_centroids)
	{
		auto result = closest(data, value_centroids);

		return value_centroids[result.index];
	}

	value_t centroid_distance(data_row_t const& data, value_row_t const& value_centroid)
	{
		return value_distance(data, value_centroid);
	}


	value_t centroid_distance(data_row_t const& data, data_row_t const& data_centroid)
	{
		return data_distance(data, data_centroid);
	}
}

