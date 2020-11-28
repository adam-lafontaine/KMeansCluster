#pragma once


#include <vector>
#include <functional>
#include <cstdint>

namespace cluster
{


	//======= CLASS DEFINITION =======================	

	class Cluster // allows for custom function to calculate distance between data and centroid
	{
	public:

		using value_t = double; // value type of centroids
		using data_t = double;

		using data_row_t = std::vector<value_t>;
		using data_row_list_t = std::vector<data_row_t>;

		using value_row_t = std::vector<value_t>;
		using value_row_list_t = std::vector<value_row_t>;

		using index_list_t = std::vector<size_t>;

		using dist_func_t = std::function<double(data_row_t const& data, value_row_t const& centroid)>;
		using to_value_funct_t = std::function<value_t(data_t data)>;


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

	private:

		dist_func_t m_distance;
		to_value_funct_t m_to_value;

		distance_result_t closest(data_row_t const& data, value_row_list_t const& value_list) const;

		cluster_result_t cluster_once(data_row_list_t const& x_list, size_t num_clusters) const;

	public:

		Cluster()
		{
			m_distance = [](data_row_t const& data, value_row_t const& centroid) { return 0.0; };
			m_to_value = [](data_t data) { return data; };
		}

		// define how distance is calculated between data and a centroid
		void set_distance(dist_func_t const& f) { m_distance = f; }

		// define how a data value is to be interpreted as if it were a centroid value
		// used when building a new centroid from a set of data
		void set_to_value(to_value_funct_t const& f) { m_to_value = f; }

		// determines clusters given the data and the number of clusters
		cluster_result_t cluster_data(data_row_list_t const& x_list, size_t num_clusters) const;

		// The index of the closest centroid for the given data row
		size_t find_centroid(data_row_t const& data, value_row_list_t const& centroids) const;
	};


	using value_t = Cluster::value_t;
	using data_t = Cluster::data_t;
	using cluster_result_t = Cluster::cluster_result_t;
	using distance_result_t = Cluster::distance_result_t;
	using data_row_t = Cluster::data_row_t;
	using data_row_list_t = Cluster::data_row_list_t;
	using value_row_t = Cluster::value_row_t;
	using value_row_list_t = Cluster::value_row_list_t;
	using index_list_t = Cluster::index_list_t;
	using dist_func_t = Cluster::dist_func_t;
	using to_value_funct_t = Cluster::to_value_funct_t;

}


