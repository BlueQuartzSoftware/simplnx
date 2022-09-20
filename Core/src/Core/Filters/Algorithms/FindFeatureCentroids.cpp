#include "FindFeatureCentroids.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

using namespace complex;

namespace
{

class FindFeatureCentroidsImpl1
{
public:
  FindFeatureCentroidsImpl1(FindFeatureCentroids* filter, double* sum, double* center, size_t* count, std::array<size_t, 3> dims, const complex::ImageGeom& imageGeom, const Int32Array& featureIds)
  : m_Filter(filter)
  , m_Sum(sum)
  , m_Center(center)
  , m_Count(count)
  , m_Dims(dims)
  , m_ImageGeom(imageGeom)
  , m_FeatureIds(featureIds)
  {
  }
  ~FindFeatureCentroidsImpl1() = default;
  void compute(int64_t minFeatureId, int64_t maxFeatureId) const
  {
    for(size_t i = 0; i < m_Dims[2]; i++)
    {
      size_t zStride = i * m_Dims[0] * m_Dims[1];
      for(size_t j = 0; j < m_Dims[1]; j++)
      {
        size_t yStride = j * m_Dims[0];
        for(size_t k = 0; k < m_Dims[0]; k++)
        {
          int32_t featureId = m_FeatureIds[zStride + yStride + k]; // Get the current FeatureId
          if(featureId < minFeatureId || featureId >= maxFeatureId)
          {
            continue;
          }
          complex::Point3Dd voxel_center = m_ImageGeom.getCoords(k, j, i); // Get the voxel center based on XYZ index from Image Geom

          // Kahan Sum for X Coord
          size_t featureId_idx = featureId * 3ULL;
          auto componentValue = static_cast<double>(voxel_center[0] - m_Center[featureId_idx]);
          double temp = m_Sum[featureId_idx] + componentValue;
          m_Center[featureId_idx] = (temp - m_Sum[featureId_idx]) - componentValue;
          m_Sum[featureId_idx] = temp;
          m_Count[featureId_idx]++;

          // Kahan Sum for Y Coord
          featureId_idx = featureId * 3ULL + 1;
          componentValue = static_cast<double>(voxel_center[1] - m_Center[featureId_idx]);
          temp = m_Sum[featureId_idx] + componentValue;
          m_Center[featureId_idx] = (temp - m_Sum[featureId_idx]) - componentValue;
          m_Sum[featureId_idx] = temp;
          m_Count[featureId_idx]++;

          // Kahan Sum for Z Coord
          featureId_idx = featureId * 3ULL + 2;
          componentValue = static_cast<double>(voxel_center[2] - m_Center[featureId_idx]);
          temp = m_Sum[featureId_idx] + componentValue;
          m_Center[featureId_idx] = (temp - m_Sum[featureId_idx]) - componentValue;
          m_Sum[featureId_idx] = temp;
          m_Count[featureId_idx]++;
        }
      }
    }
  }

  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  FindFeatureCentroids* m_Filter = nullptr;
  double* m_Sum = nullptr;
  double* m_Center = nullptr;
  size_t* m_Count = nullptr;
  size_t m_TotalFeatures = 0;
  std::array<size_t, 3> m_Dims = {0, 0, 0};
  const complex::ImageGeom& m_ImageGeom;
  const Int32Array& m_FeatureIds;
};

} // namespace

// -----------------------------------------------------------------------------
FindFeatureCentroids::FindFeatureCentroids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindFeatureCentroidsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindFeatureCentroids::~FindFeatureCentroids() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindFeatureCentroids::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindFeatureCentroids::operator()()
{
  // Input Cell Data
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);

  // Output Feature Data
  auto& centroids = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CentroidsArrayPath);

  // Required Geometry
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);

  size_t totalFeatures = centroids.getNumberOfTuples();

  size_t xPoints = imageGeom.getNumXCells();
  size_t yPoints = imageGeom.getNumYCells();
  size_t zPoints = imageGeom.getNumZCells();

  std::vector<double> sum(totalFeatures * 3, 0.0);
  std::vector<double> center(totalFeatures * 3, 0.0);
  std::vector<size_t> count(totalFeatures * 3, 0.0);

  // The first part can be expensive so parallelize the algorithm
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, totalFeatures);
  dataAlg.setParallelizationEnabled(true);
  dataAlg.execute(FindFeatureCentroidsImpl1(this, sum.data(), center.data(), count.data(), {xPoints, yPoints, zPoints}, imageGeom, featureIds));

  // Here we are only looping over the number of features so let this just go in serial mode.
  for(size_t featureId = 0; featureId < totalFeatures; featureId++)
  {
    auto featureId_idx = static_cast<size_t>(featureId * 3);
    if(static_cast<float>(count[featureId_idx]) > 0.0f)
    {
      centroids[featureId_idx] = static_cast<float>(sum[featureId_idx] / static_cast<double>(count[featureId_idx]));
    }

    featureId_idx = static_cast<size_t>(featureId * 3 + 1);
    if(static_cast<float>(count[featureId_idx]) > 0.0f)
    {
      centroids[featureId_idx] = static_cast<float>(sum[featureId_idx] / static_cast<double>(count[featureId_idx]));
    }

    featureId_idx = static_cast<size_t>(featureId * 3 + 2);
    if(static_cast<float>(count[featureId_idx]) > 0.0f)
    {
      centroids[featureId_idx] = static_cast<float>(sum[featureId_idx] / static_cast<double>(count[featureId_idx]));
    }
  }

  return {};
}
