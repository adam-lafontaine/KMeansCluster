#pragma once


#include <vector>
#include <functional>
#include <cstdint>

namespace cluster
{
	//======= TYPE DEFINITIONS ====================

	using value_t = double; // value type of centroids
	using data_t = uint8_t; // shade_t

	using data_row_t = std::vector<data_t>;
	using data_row_list_t = std::vector<data_row_t>;

	using value_row_t = std::vector<value_t>;
	using value_row_list_t = std::vector<value_row_t>;

	using index_list_t = std::vector<size_t>;

	using dist_func_t = std::function<double(data_row_t const& data, value_row_t const& centroid)>;


	typedef struct ClusterResult
	{
		index_list_t x_clusters;      // the cluster index of each data point
		value_row_list_t centroids;   // centroids found
		value_t average_distance = 0; // 

	} cluster_result_t;

	
	typedef struct DistanceResult 
	{
		size_t index;    // index of centroid in the list
		double distance; // calculated distance of data from the centroid

	} distance_result_t;


	//======= CLASS DEFINITION =======================	

	class Cluster // allows for custom function to calculate distance between data and centroid
	{
	private:

		dist_func_t m_dist_func;

		distance_result_t closest(data_row_t const& data, value_row_list_t const& value_list) const;

		cluster_result_t cluster_once(data_row_list_t const& x_list, size_t num_clusters) const;

	public:

		Cluster() : m_dist_func([](data_row_t const& data, value_row_t const& centroid) { return 0.0; }) {}

		void set_distance(dist_func_t const& f) { m_dist_func = f; }

		// determines clusters given the data and the number of clusters
		value_row_list_t cluster_data(data_row_list_t const& x_list, size_t num_clusters) const;

		// The index of the closest centroid in the list
		size_t find_centroid(data_row_t const& data, value_row_list_t const& centroids) const;
	};

}


