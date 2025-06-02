#include "DBSCAN.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <algorithm>
#include <fmt/format.h>
#include <queue>
#include <unordered_map>

using namespace nx::core;

namespace
{

template <typename T>
class GridSpatialIndex
{
private:
  using AbstractDataStoreT = AbstractDataStore<T>;

  std::unordered_map<size_t, std::vector<usize>> grid_cells;
  float64 cell_size;
  usize num_dimensions;
  std::vector<T> min_bounds, max_bounds;
  const AbstractDataStoreT& data_store;
  usize num_components;

  size_t hash_cell_coordinates(const std::vector<int>& coordinates) const noexcept
  {
    size_t hash_value = 0;
    const size_t prime_base = 73856093;

    for(usize i = 0; i < coordinates.size(); ++i)
    {
      hash_value ^= static_cast<size_t>(coordinates[i]) * (prime_base + i);
    }
    return hash_value;
  }

  std::vector<int> calculate_cell_coordinates(usize point_index) const noexcept
  {
    std::vector<int> coordinates(num_dimensions);

    for(usize dim = 0; dim < num_dimensions; ++dim)
    {
      T point_value = data_store[point_index * num_components + dim];
      coordinates[dim] = static_cast<int>((point_value - min_bounds[dim]) / cell_size);
    }
    return coordinates;
  }

public:
  GridSpatialIndex(const AbstractDataStoreT& input_data, usize num_comps, usize num_tuples, float64 epsilon, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask)
  : data_store(input_data)
  , num_components(num_comps)
  , cell_size(epsilon)
  , num_dimensions(num_comps)
  {
    if(num_tuples == 0)
      return;

    // Calculate data bounds
    min_bounds.resize(num_dimensions);
    max_bounds.resize(num_dimensions);

    bool bounds_initialized = false;

    for(usize i = 0; i < num_tuples; ++i)
    {
      if(mask->isTrue(i))
      {
        for(usize dim = 0; dim < num_dimensions; ++dim)
        {
          T value = data_store[i * num_components + dim];

          if(!bounds_initialized)
          {
            min_bounds[dim] = max_bounds[dim] = value;
          }
          else
          {
            min_bounds[dim] = std::min(min_bounds[dim], value);
            max_bounds[dim] = std::max(max_bounds[dim], value);
          }
        }
        bounds_initialized = true;
      }
    }

    // Populate grid cells
    for(usize i = 0; i < num_tuples; ++i)
    {
      if(mask->isTrue(i))
      {
        auto coordinates = calculate_cell_coordinates(i);
        size_t hash_key = hash_cell_coordinates(coordinates);
        grid_cells[hash_key].push_back(i);
      }
    }
  }

  std::vector<usize> find_neighbors(usize query_index, float64 epsilon_value, ClusterUtilities::DistanceMetric distance_metric) const
  {
    std::vector<usize> neighbors;
    auto center_coordinates = calculate_cell_coordinates(query_index);

    // Check all neighboring grid cells (including center cell)
    std::function<void(std::vector<int>, usize)> check_neighboring_cells;
    check_neighboring_cells = [&](std::vector<int> current_coords, usize dimension_index) {
      if(dimension_index == num_dimensions)
      {
        size_t cell_hash = hash_cell_coordinates(current_coords);
        auto cell_iterator = grid_cells.find(cell_hash);

        if(cell_iterator != grid_cells.end())
        {
          for(usize candidate_index : cell_iterator->second)
          {
            float64 distance = ClusterUtilities::GetDistance(data_store, query_index * num_components, data_store, candidate_index * num_components, num_components, distance_metric);

            if(distance < epsilon_value)
            {
              neighbors.push_back(candidate_index);
            }
          }
        }
        return;
      }

      // Check current cell and adjacent cells in current dimension
      for(int offset = -1; offset <= 1; ++offset)
      {
        current_coords[dimension_index] = center_coordinates[dimension_index] + offset;
        check_neighboring_cells(current_coords, dimension_index + 1);
      }
    };

    std::vector<int> coordinates(num_dimensions);
    check_neighboring_cells(coordinates, 0);
    return neighbors;
  }
};

template <typename T>
class OptimizedDBSCANTemplate
{
private:
  using AbstractDataStoreT = AbstractDataStore<T>;

public:
  OptimizedDBSCANTemplate(DBSCAN* filter, const AbstractDataStoreT& input_data, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask_array, AbstractDataStore<int32>& feature_ids,
                          float32 epsilon, int32 min_points, ClusterUtilities::DistanceMetric distance_metric, std::mt19937_64::result_type seed)
  : m_Filter(filter)
  , m_InputDataStore(input_data)
  , m_Mask(mask_array)
  , m_FeatureIds(feature_ids)
  , m_Epsilon(epsilon)
  , m_MinPoints(min_points)
  , m_DistMetric(distance_metric)
  , m_Seed(seed)
  {
  }

  ~OptimizedDBSCANTemplate() = default;

  OptimizedDBSCANTemplate(const OptimizedDBSCANTemplate&) = delete;
  void operator=(const OptimizedDBSCANTemplate&) = delete;

  void operator()()
  {
    usize num_tuples = m_InputDataStore.getNumberOfTuples();
    usize num_components = m_InputDataStore.getNumberOfComponents();

    // Initialize point states efficiently
    std::vector<bool> visited(num_tuples, false);
    std::vector<bool> is_core_point(num_tuples, false);

    // Initialize all feature IDs to noise (-1 becomes 0 for noise)
    std::fill(m_FeatureIds.begin(), m_FeatureIds.end(), 0);

    float64 epsilon_value = static_cast<float64>(m_Epsilon);
    int32 current_cluster_id = 1; // Start from 1, 0 is reserved for noise

    m_Filter->updateProgress("Building spatial index...");

    // Build spatial index for efficient neighbor finding
    GridSpatialIndex<T> spatial_index(m_InputDataStore, num_components, num_tuples, epsilon_value, m_Mask);

    m_Filter->updateProgress("Starting clustering process...");
    auto clustering_start_time = std::chrono::steady_clock::now();

    // Process each point
    for(usize point_index = 0; point_index < num_tuples; ++point_index)
    {
      if(m_Filter->getCancel())
      {
        return;
      }

      // Skip if already visited or masked out
      if(visited[point_index] || !m_Mask->isTrue(point_index))
      {
        continue;
      }

      visited[point_index] = true;

      // Progress reporting every 1000 points or every second
      if(point_index % 1000 == 0)
      {
        auto current_time = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::seconds>(current_time - clustering_start_time).count() >= 1)
        {
          float32 progress_percentage = (static_cast<float32>(point_index) / static_cast<float32>(num_tuples)) * 100.0f;
          m_Filter->updateProgress(fmt::format("Processing point {} of {} ({:.1f}% complete)", point_index, num_tuples, progress_percentage));
          clustering_start_time = current_time;
        }
      }

      // Find all neighbors within epsilon distance
      auto neighbors = spatial_index.find_neighbors(point_index, epsilon_value, m_DistMetric);

      // Check if this is a core point
      if(static_cast<int32>(neighbors.size()) < m_MinPoints)
      {
        // Not enough neighbors - remains noise (feature ID = 0)
        continue;
      }

      // This is a core point - start a new cluster
      is_core_point[point_index] = true;
      m_FeatureIds[point_index] = current_cluster_id;

      // Expand cluster using queue-based approach
      std::queue<usize> expansion_queue;

      // Add all unvisited neighbors to expansion queue
      for(usize neighbor_index : neighbors)
      {
        if(m_Mask->isTrue(neighbor_index))
        {
          if(!visited[neighbor_index])
          {
            visited[neighbor_index] = true;
            expansion_queue.push(neighbor_index);
          }

          // Assign to cluster if not already assigned
          if(m_FeatureIds[neighbor_index] == 0)
          {
            m_FeatureIds[neighbor_index] = current_cluster_id;
          }
        }
      }

      // Expand cluster by processing queue
      while(!expansion_queue.empty())
      {
        if(m_Filter->getCancel())
        {
          return;
        }

        usize current_point = expansion_queue.front();
        expansion_queue.pop();

        // Find neighbors of current point
        auto current_neighbors = spatial_index.find_neighbors(current_point, epsilon_value, m_DistMetric);

        // If current point is also a core point, add its unvisited neighbors to queue
        if(static_cast<int32>(current_neighbors.size()) >= m_MinPoints)
        {
          is_core_point[current_point] = true;

          for(usize neighbor_idx : current_neighbors)
          {
            if(m_Mask->isTrue(neighbor_idx))
            {
              if(!visited[neighbor_idx])
              {
                visited[neighbor_idx] = true;
                expansion_queue.push(neighbor_idx);
              }

              // Assign to cluster if it's noise
              if(m_FeatureIds[neighbor_idx] == 0)
              {
                m_FeatureIds[neighbor_idx] = current_cluster_id;
              }
            }
          }
        }
      }

      ++current_cluster_id;
    }

    m_Filter->updateProgress(fmt::format("Clustering complete! Found {} clusters.", current_cluster_id - 1));
  }

private:
  DBSCAN* m_Filter;
  const AbstractDataStoreT& m_InputDataStore;
  const std::unique_ptr<MaskCompareUtilities::MaskCompare>& m_Mask;
  AbstractDataStore<int32>& m_FeatureIds;
  float32 m_Epsilon;
  int32 m_MinPoints;
  ClusterUtilities::DistanceMetric m_DistMetric;
  std::mt19937_64::result_type m_Seed;
};

struct OptimizedDBSCANFunctor
{
  template <typename T>
  void operator()(bool cache, bool use_random, DBSCAN* filter, const IDataArray& input_array, const std::unique_ptr<MaskCompareUtilities::MaskCompare>& mask_compare, Int32Array& feature_ids,
                  float32 epsilon, int32 min_points, ClusterUtilities::DistanceMetric distance_metric, std::mt19937_64::result_type seed)
  {
    // Note: cache and use_random parameters are ignored in optimized version
    // The new implementation uses spatial indexing which is more efficient than caching
    OptimizedDBSCANTemplate<T>(filter, input_array.template getIDataStoreRefAs<AbstractDataStore<T>>(), mask_compare, feature_ids.getDataStoreRef(), epsilon, min_points, distance_metric, seed)();
  }
};

} // namespace

// -----------------------------------------------------------------------------
DBSCAN::DBSCAN(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DBSCANInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
DBSCAN::~DBSCAN() noexcept = default;

// -----------------------------------------------------------------------------
void DBSCAN::updateProgress(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

// -----------------------------------------------------------------------------
const std::atomic_bool& DBSCAN::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> DBSCAN::operator()()
{
  auto& clustering_array = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->ClusteringArrayPath);
  auto& feature_ids = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);

  std::unique_ptr<MaskCompareUtilities::MaskCompare> mask_compare;
  try
  {
    mask_compare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    std::string error_message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
    return MakeErrorResult(-54060, error_message);
  }

  // Execute optimized DBSCAN algorithm
  ExecuteNeighborFunction(OptimizedDBSCANFunctor{}, clustering_array.getDataType(), m_InputValues->AllowCaching, m_InputValues->UseRandom, this, clustering_array, mask_compare, feature_ids,
                          m_InputValues->Epsilon, m_InputValues->MinPoints, m_InputValues->DistanceMetric, m_InputValues->Seed);

  updateProgress("Resizing clustering attribute matrix...");

  auto& feature_ids_store = feature_ids.getDataStoreRef();
  int32 max_cluster_id = *std::max_element(feature_ids_store.begin(), feature_ids_store.end());
  m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->FeatureAM)->resizeTuples(AttributeMatrix::ShapeType{static_cast<usize>(max_cluster_id + 1)});

  return {};
}