#pragma once
#include <vector>
#include <string>

namespace cluster
{
	//======= TYPE DEFINITIONS ====================

	using value_t = double;
	using data_t = char;

	using data_row_t = std::string;
	using data_row_list_t = std::vector<data_row_t>;

	using value_row_t = std::vector<value_t>;
	using value_row_list_t = std::vector<value_row_t>;


	using index_list_t = std::vector<size_t>;

	typedef struct ClusterResult {
		index_list_t x_clusters;
		value_row_list_t centroids;
		value_t average_distance = 0;

	} cluster_result_t;


	//======= CLUSTER ALGORITHMS =========================

	// returns the result with the smallest distance
	cluster_result_t cluster_min_distance(data_row_list_t const& x_list, size_t num_clusters);

	// returns the most popular result
	// stops when the same result has been found for more than half of the attempts
	cluster_result_t cluster_max_count(data_row_list_t const& x_list, size_t num_clusters);

	// keeps increasing the number of clusters until the incremental improvement is small enough
	cluster_result_t cluster_unknown(data_row_list_t const& x_list, size_t min_clusters, size_t max_clusters);


	//======= DATA ===================


	// convert a list of value_row_t to data_row_t
	data_row_list_t to_data_row_list(value_row_list_t const& value_row_list);

	// convert a list of data_row_t to value_row_t
	value_row_list_t to_value_row_list(data_row_list_t const& data_row_list);

	// finds centroid closest to data
	value_row_t find_centroid(data_row_t const& data, value_row_list_t const& value_centroids);

	value_t centroid_distance(data_row_t const& data, data_row_t const& data_centroid);


	// distance of data from a given centroid
	value_t centroid_distance(data_row_t const& data, value_row_t const& value_centroid);

	//======= TESTING ======================

	// for trying to find the best number of clusters
	void find_clusters(data_row_list_t const& x_list, size_t max_clusters);
}