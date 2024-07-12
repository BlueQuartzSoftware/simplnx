#include "ComputeEuclideanDistMap.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

using namespace nx::core;

namespace
{
/**
 * @brief The ComputeDistanceMapImpl class implements a threaded algorithm that computes the  distance map
 * for each point in the supplied volume
 */
template <typename T>
class ComputeDistanceMapImpl
{
  DataStructure& m_DataStructure;
  const ComputeEuclideanDistMapInputValues& m_InputValues;
  ComputeEuclideanDistMap::MapType m_MapType;

public:
  ComputeDistanceMapImpl(DataStructure& dataStructure, const ComputeEuclideanDistMapInputValues& inputValues, ComputeEuclideanDistMap::MapType mapType)
  : m_DataStructure(dataStructure)
  , m_InputValues(inputValues)
  , m_MapType(mapType)
  {
  }

  virtual ~ComputeDistanceMapImpl() = default;

  void operator()() const
  {
    using DataArrayType = DataArray<T>;

    const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues.InputImageGeometry);

    SizeVec3 udims = selectedImageGeom.getDimensions();

    size_t totalPoints = selectedImageGeom.getNumberOfCells();
    double Distance = 0.0;
    size_t count = 1;
    size_t changed = 1;
    size_t neighpoint = 0;
    int64_t nearestneighbor;
    int64_t neighbors[6] = {0, 0, 0, 0, 0, 0};
    auto xpoints = static_cast<int64_t>(udims[0]);
    auto ypoints = static_cast<int64_t>(udims[1]);
    auto zpoints = static_cast<int64_t>(udims[2]);
    FloatVec3 spacing = selectedImageGeom.getSpacing();

    neighbors[0] = -xpoints * ypoints;
    neighbors[1] = -xpoints;
    neighbors[2] = -1;
    neighbors[3] = 1;
    neighbors[4] = xpoints;
    neighbors[5] = xpoints * ypoints;

    // Use a std::vector to get an auto cleaned up array thus not needing the 'delete' keyword later on.
    std::vector<int32_t> voxNN(totalPoints, 0);
    int32_t* voxel_NearestNeighbor = &(voxNN.front());
    std::vector<double> voxEDist(totalPoints, 0.0);
    double* voxel_Distance = &(voxEDist.front());

    const auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues.FeatureIdsArrayPath)->getDataStoreRef();

    auto* gbManhattanDistancesStore = m_DataStructure.template getDataAs<DataArrayType>(m_InputValues.GBDistancesArrayName)->getDataStore();
    auto* tjManhattanDistancesStore = m_DataStructure.template getDataAs<DataArrayType>(m_InputValues.TJDistancesArrayName)->getDataStore();
    auto* qpManhattanDistancesStore = m_DataStructure.template getDataAs<DataArrayType>(m_InputValues.QPDistancesArrayName)->getDataStore();

    auto* nearestNeighborsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues.NearestNeighborsArrayName)->getDataStore();

    Distance = 0;
    for(size_t a = 0; a < totalPoints; ++a)
    {
      if((*nearestNeighborsStore)[a * 3 + static_cast<uint32_t>(m_MapType)] >= 0)
      {
        voxel_NearestNeighbor[a] = static_cast<int32_t>(a);
      } // if voxel is boundary voxel, then want to use itself as nearest boundary voxel
      else
      {
        voxel_NearestNeighbor[a] = -1;
      }
      if(m_MapType == ComputeEuclideanDistMap::MapType::FeatureBoundary)
      {
        voxel_Distance[a] = static_cast<double>((*gbManhattanDistancesStore)[a]);
      }
      else if(m_MapType == ComputeEuclideanDistMap::MapType::TripleJunction)
      {
        voxel_Distance[a] = static_cast<double>((*tjManhattanDistancesStore)[a]);
      }
      else if(m_MapType == ComputeEuclideanDistMap::MapType::QuadPoint)
      {
        voxel_Distance[a] = static_cast<double>((*qpManhattanDistancesStore)[a]);
      }
    }

    // ------------- Calculate the Manhattan Distance ----------------
    count = 1;
    changed = 1;
    int64_t i = 0;
    int64_t zBlock = xpoints * ypoints;
    int64_t zStride = 0, yStride = 0;
    char mask[6] = {0, 0, 0, 0, 0, 0};
    while(count > 0 && changed > 0)
    {
      count = 0;
      changed = 0;
      Distance++;

      for(int64_t z = 0; z < zpoints; ++z)
      {
        zStride = z * zBlock;
        mask[0] = mask[5] = 1;
        if(z == 0)
        {
          mask[0] = 0;
        }
        if(z == zpoints - 1)
        {
          mask[5] = 0;
        }

        for(int64_t y = 0; y < ypoints; ++y)
        {
          yStride = y * xpoints;
          mask[1] = mask[4] = 1;
          if(y == 0)
          {
            mask[1] = 0;
          }
          if(y == ypoints - 1)
          {
            mask[4] = 0;
          }

          for(int64_t x = 0; x < xpoints; ++x)
          {
            mask[2] = mask[3] = 1;
            if(x == 0)
            {
              mask[2] = 0;
            }
            if(x == xpoints - 1)
            {
              mask[3] = 0;
            }

            i = zStride + yStride + x;
            if(voxel_NearestNeighbor[i] == -1 && featureIdsStore[i] > 0)
            {
              count++;
              for(int32_t j = 0; j < 6; j++)
              {
                neighpoint = i + neighbors[j];
                if(mask[j] == 1)
                {
                  if(voxel_Distance[neighpoint] != -1.0)
                  {
                    voxel_NearestNeighbor[i] = voxel_NearestNeighbor[neighpoint];
                  }
                }
              }
            }
          }
        }
      }
      for(size_t j = 0; j < totalPoints; ++j)
      {
        if(voxel_NearestNeighbor[j] != -1 && voxel_Distance[j] == -1.0 && featureIdsStore[j] > 0)
        {
          changed++;
          voxel_Distance[j] = Distance;
        }
      }
    }

    // ------------- Calculate the Euclidian Distance ----------------

    if(!m_InputValues.CalcManhattanDist)
    {
      double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0, z1 = 0.0, z2 = 0.0;
      double dist = 0.0;
      double oneOverzBlock = 1.0 / double(zBlock);
      double oneOverxpoints = 1.0 / double(xpoints);
      for(int64_t m = 0; m < zpoints; m++)
      {
        zStride = m * zBlock;
        for(int64_t n = 0; n < ypoints; n++)
        {
          yStride = n * xpoints;
          for(int64_t p = 0; p < xpoints; p++)
          {
            x1 = static_cast<double>(p) * spacing[0];
            y1 = static_cast<double>(n) * spacing[1];
            z1 = static_cast<double>(m) * spacing[2];
            nearestneighbor = voxel_NearestNeighbor[zStride + yStride + p];
            if(nearestneighbor >= 0)
            {
              x2 = spacing[0] * static_cast<double>(nearestneighbor % xpoints);                                        // find_xcoord(nearestneighbor);
              y2 = spacing[1] * static_cast<double>(static_cast<int64_t>(nearestneighbor * oneOverxpoints) % ypoints); // find_ycoord(nearestneighbor);
              z2 = spacing[2] * floor(nearestneighbor * oneOverzBlock);                                                // find_zcoord(nearestneighbor);
              dist = ((x1 - x2) * (x1 - x2)) + ((y1 - y2) * (y1 - y2)) + ((z1 - z2) * (z1 - z2));
              dist = sqrt(dist);
              voxel_Distance[zStride + yStride + p] = dist;
            }
          }
        }
      }
    }
    for(size_t a = 0; a < totalPoints; ++a)
    {
      (*nearestNeighborsStore)[a * 3 + static_cast<uint32_t>(m_MapType)] = voxel_NearestNeighbor[a];
      if(m_MapType == ComputeEuclideanDistMap::MapType::FeatureBoundary)
      {
        (*gbManhattanDistancesStore)[a] = static_cast<T>(voxel_Distance[a]);
      }
      else if(m_MapType == ComputeEuclideanDistMap::MapType::TripleJunction)
      {
        (*tjManhattanDistancesStore)[a] = static_cast<T>(voxel_Distance[a]);
      }
      else if(m_MapType == ComputeEuclideanDistMap::MapType::QuadPoint)
      {
        (*qpManhattanDistancesStore)[a] = static_cast<T>(voxel_Distance[a]);
      }
    }
  }
};
} // namespace

// -----------------------------------------------------------------------------
ComputeEuclideanDistMap::ComputeEuclideanDistMap(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ComputeEuclideanDistMapInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeEuclideanDistMap::~ComputeEuclideanDistMap() noexcept = default;

// -----------------------------------------------------------------------------
template <typename T>
void findDistanceMap(DataStructure& dataStructure, const ComputeEuclideanDistMapInputValues* inputValues)
{
  using DataArrayType = DataArray<T>;

  const auto& featureIdsStore = dataStructure.getDataRefAs<Int32Array>(inputValues->FeatureIdsArrayPath).getDataStoreRef();
  size_t totalPoints = featureIdsStore.getNumberOfTuples();

  auto* gbManhattanDistancesStore = dataStructure.template getDataAs<DataArrayType>(inputValues->GBDistancesArrayName)->getDataStore();
  if(gbManhattanDistancesStore != nullptr)
  {
    gbManhattanDistancesStore->fill(static_cast<T>(-1));
  }

  auto* tjManhattanDistancesStore = dataStructure.template getDataAs<DataArrayType>(inputValues->TJDistancesArrayName)->getDataStore();
  if(tjManhattanDistancesStore != nullptr)
  {
    tjManhattanDistancesStore->fill(static_cast<T>(-1));
  }

  auto* qpManhattanDistancesStore = dataStructure.template getDataAs<DataArrayType>(inputValues->QPDistancesArrayName)->getDataStore();
  if(qpManhattanDistancesStore != nullptr)
  {
    qpManhattanDistancesStore->fill(static_cast<T>(-1));
  }

  auto* nearestNeighbors = dataStructure.template getDataAs<Int32Array>(inputValues->NearestNeighborsArrayName);
  if(nearestNeighbors != nullptr)
  {
    nearestNeighbors->fill(static_cast<T>(-1));
  }

  const auto& selectedImageGeom = dataStructure.getDataRefAs<ImageGeom>(inputValues->InputImageGeometry);
  SizeVec3 udims = selectedImageGeom.getDimensions();
  std::array<int64_t, 3> dims = {
      static_cast<int64_t>(udims[0]),
      static_cast<int64_t>(udims[1]),
      static_cast<int64_t>(udims[2]),
  };

  int64_t column = 0, row = 0, plane = 0;
  bool good = false;
  bool add = true;
  int32_t feature = 0;
  std::vector<int32_t> coordination;

  int64_t neighbor = 0;
  int64_t neighbors[6] = {0, 0, 0, 0, 0, 0};
  neighbors[0] = -dims[0] * dims[1];
  neighbors[1] = -dims[0];
  neighbors[2] = -1;
  neighbors[3] = 1;
  neighbors[4] = dims[0];
  neighbors[5] = dims[0] * dims[1];

  size_t xPoints = udims[0];
  size_t yPoints = udims[1];
  size_t zPoints = udims[2];

  for(size_t a = 0; a < totalPoints; ++a)
  {
    feature = featureIdsStore[a];
    if(feature > 0)
    {
      column = static_cast<int64_t>(a % xPoints);
      row = static_cast<int64_t>((a / xPoints) % yPoints);
      plane = static_cast<int64_t>(a / (xPoints * yPoints));
      for(int32_t k = 0; k < 6; k++)
      {
        good = true;
        neighbor = static_cast<int64_t>(a + neighbors[k]);
        if(k == 0 && plane == 0)
        {
          good = false;
        }
        if(k == 5 && plane == static_cast<int64_t>(zPoints - 1))
        {
          good = false;
        }
        if(k == 1 && row == 0)
        {
          good = false;
        }
        if(k == 4 && row == static_cast<int64_t>(yPoints - 1))
        {
          good = false;
        }
        if(k == 2 && column == 0)
        {
          good = false;
        }
        if(k == 3 && column == static_cast<int64_t>(xPoints - 1))
        {
          good = false;
        }
        if(good && featureIdsStore[neighbor] != feature && featureIdsStore[neighbor] >= 0)
        {
          add = true;
          for(const auto& coordination_value : coordination)
          {
            if(featureIdsStore[neighbor] == coordination_value)
            {
              add = false;
              break;
            }
          }
          if(add)
          {
            coordination.push_back(featureIdsStore[neighbor]);
          }
        }
      }
      if(coordination.empty())
      {
        (*nearestNeighbors)[a * 3 + 0] = -1;
        (*nearestNeighbors)[a * 3 + 1] = -1;
        (*nearestNeighbors)[a * 3 + 2] = -1;
      }
      if(!coordination.empty() && inputValues->DoBoundaries)
      {
        (*gbManhattanDistancesStore)[a] = 0;
        (*nearestNeighbors)[a * 3 + 0] = coordination[0];
        (*nearestNeighbors)[a * 3 + 1] = -1;
        (*nearestNeighbors)[a * 3 + 2] = -1;
      }
      if(coordination.size() >= 2 && inputValues->DoTripleLines)
      {
        (*tjManhattanDistancesStore)[a] = 0;
        (*nearestNeighbors)[a * 3 + 0] = coordination[0];
        (*nearestNeighbors)[a * 3 + 1] = coordination[0];
        (*nearestNeighbors)[a * 3 + 2] = -1;
      }
      if(coordination.size() > 2 && inputValues->DoQuadPoints)
      {
        (*qpManhattanDistancesStore)[a] = 0;
        (*nearestNeighbors)[a * 3 + 0] = coordination[0];
        (*nearestNeighbors)[a * 3 + 1] = coordination[0];
        (*nearestNeighbors)[a * 3 + 2] = coordination[0];
      }
      coordination.resize(0);
    }
  }

  ParallelTaskAlgorithm taskRunner;
  if(inputValues->DoBoundaries)
  {
    if(inputValues->CalcManhattanDist)
    {
      taskRunner.execute(ComputeDistanceMapImpl<int32>(dataStructure, *inputValues, ComputeEuclideanDistMap::MapType::FeatureBoundary));
    }
    else
    {
      taskRunner.execute(ComputeDistanceMapImpl<float32>(dataStructure, *inputValues, ComputeEuclideanDistMap::MapType::FeatureBoundary));
    }
  }

  if(inputValues->DoTripleLines)
  {
    if(inputValues->CalcManhattanDist)
    {
      taskRunner.execute(ComputeDistanceMapImpl<int32>(dataStructure, *inputValues, ComputeEuclideanDistMap::MapType::TripleJunction));
    }
    else
    {
      taskRunner.execute(ComputeDistanceMapImpl<float32>(dataStructure, *inputValues, ComputeEuclideanDistMap::MapType::TripleJunction));
    }
  }
  if(inputValues->DoQuadPoints)
  {
    if(inputValues->CalcManhattanDist)
    {
      taskRunner.execute(ComputeDistanceMapImpl<int32>(dataStructure, *inputValues, ComputeEuclideanDistMap::MapType::QuadPoint));
    }
    else
    {
      taskRunner.execute(ComputeDistanceMapImpl<float32>(dataStructure, *inputValues, ComputeEuclideanDistMap::MapType::QuadPoint));
    }
  }
  // Wait for tasks to complete
  taskRunner.wait();
}

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeEuclideanDistMap::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeEuclideanDistMap::operator()()
{
  if(m_InputValues->CalcManhattanDist)
  {
    findDistanceMap<int32>(m_DataStructure, m_InputValues);
  }
  else
  {
    findDistanceMap<float32>(m_DataStructure, m_InputValues);
  }

  return {};
}
