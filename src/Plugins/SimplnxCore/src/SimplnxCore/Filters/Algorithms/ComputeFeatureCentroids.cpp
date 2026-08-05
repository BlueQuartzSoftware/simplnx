#include "ComputeFeatureCentroids.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <algorithm>

using namespace nx::core;

namespace
{
// Computes per-feature centroids as the mean of the voxel-center coordinates of every cell in the
// feature, using Kahan compensated summation to limit float round-off on large features. m_Sum holds
// the running per-component sum; m_Compensation holds the Kahan compensation (the low-order bits lost
// on the previous add) — it is NOT a center. The centroid is produced later as m_Sum / m_Count.
class ComputeFeatureCentroidsImpl1
{
public:
  ComputeFeatureCentroidsImpl1(Float64AbstractDataStore& sum, Float64AbstractDataStore& compensation, UInt64AbstractDataStore& count, std::array<size_t, 3> dims, const nx::core::ImageGeom& imageGeom,
                               const Int32AbstractDataStore& featureIds, UInt64AbstractDataStore& rangeXStoreRef, UInt64AbstractDataStore& rangeYStoreRef, UInt64AbstractDataStore& rangeZStoreRef)
  : m_Sum(sum)
  , m_Compensation(compensation)
  , m_Count(count)
  , m_Dims(dims)
  , m_ImageGeom(imageGeom)
  , m_FeatureIds(featureIds)
  , m_RangeXStoreRef(rangeXStoreRef)
  , m_RangeYStoreRef(rangeYStoreRef)
  , m_RangeZStoreRef(rangeZStoreRef)
  {
  }
  ~ComputeFeatureCentroidsImpl1() = default;
  void compute(usize minFeatureId, usize maxFeatureId) const
  {
    for(uint64 i = 0; i < m_Dims[2]; i++)
    {
      size_t zStride = i * m_Dims[0] * m_Dims[1];
      for(uint64 j = 0; j < m_Dims[1]; j++)
      {
        size_t yStride = j * m_Dims[0];
        for(uint64 k = 0; k < m_Dims[0]; k++)
        {
          int32 featureId = m_FeatureIds[zStride + yStride + k]; // Get the current FeatureId
          if(featureId < minFeatureId || featureId >= maxFeatureId)
          {
            continue;
          }
          // Check if feature ID is Periodic
          m_RangeXStoreRef[featureId * 2 + 0] = std::min(k, m_RangeXStoreRef.getValue(featureId * 2 + 0));
          m_RangeXStoreRef[featureId * 2 + 1] = std::max(k, m_RangeXStoreRef.getValue(featureId * 2 + 1));

          m_RangeYStoreRef[featureId * 2 + 0] = std::min(j, m_RangeYStoreRef.getValue(featureId * 2 + 0));
          m_RangeYStoreRef[featureId * 2 + 1] = std::max(j, m_RangeYStoreRef.getValue(featureId * 2 + 1));

          m_RangeZStoreRef[featureId * 2 + 0] = std::min(i, m_RangeZStoreRef.getValue(featureId * 2 + 0));
          m_RangeZStoreRef[featureId * 2 + 1] = std::max(i, m_RangeZStoreRef.getValue(featureId * 2 + 1));

          // Get the voxel center based on XYZ index from Image Geom
          nx::core::Point3Dd voxel_center = m_ImageGeom.getCoords(k, j, i);

          // Kahan Sum for X Coord
          size_t featureId_idx = featureId * 3ULL;
          auto componentValue = static_cast<double>(voxel_center[0] - m_Compensation[featureId_idx]);
          double temp = m_Sum[featureId_idx] + componentValue;
          m_Compensation[featureId_idx] = (temp - m_Sum[featureId_idx]) - componentValue;
          m_Sum[featureId_idx] = temp;
          m_Count[featureId_idx].inc();

          // Kahan Sum for Y Coord
          featureId_idx = featureId * 3ULL + 1;
          componentValue = static_cast<double>(voxel_center[1] - m_Compensation[featureId_idx]);
          temp = m_Sum[featureId_idx] + componentValue;
          m_Compensation[featureId_idx] = (temp - m_Sum[featureId_idx]) - componentValue;
          m_Sum[featureId_idx] = temp;
          m_Count[featureId_idx].inc();

          // Kahan Sum for Z Coord
          featureId_idx = featureId * 3ULL + 2;
          componentValue = static_cast<double>(voxel_center[2] - m_Compensation[featureId_idx]);
          temp = m_Sum[featureId_idx] + componentValue;
          m_Compensation[featureId_idx] = (temp - m_Sum[featureId_idx]) - componentValue;
          m_Sum[featureId_idx] = temp;
          m_Count[featureId_idx].inc();
        }
      }
    }
  }

  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  Float64AbstractDataStore& m_Sum;
  Float64AbstractDataStore& m_Compensation;
  UInt64AbstractDataStore& m_Count;
  std::array<size_t, 3> m_Dims = {0, 0, 0};
  const nx::core::ImageGeom& m_ImageGeom;
  const Int32AbstractDataStore& m_FeatureIds;
  UInt64AbstractDataStore& m_RangeXStoreRef;
  UInt64AbstractDataStore& m_RangeYStoreRef;
  UInt64AbstractDataStore& m_RangeZStoreRef;
};

} // namespace

// -----------------------------------------------------------------------------
ComputeFeatureCentroids::ComputeFeatureCentroids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ComputeFeatureCentroidsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureCentroids::~ComputeFeatureCentroids() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeFeatureCentroids::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeFeatureCentroids::operator()()
{
  // Input Cell Data

  const auto* featureIdsPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureIdsStoreRef = featureIdsPtr->getDataStoreRef();

  // Output Feature Data
  auto& centroids = m_DataStructure.getDataAs<Float32Array>(m_InputValues->CentroidsArrayPath)->getDataStoreRef();

  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->CentroidsArrayPath, *featureIdsPtr, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }

  // Required Geometry
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);

  size_t totalFeatures = centroids.getNumberOfTuples();

  size_t xPoints = imageGeom.getNumXCells();
  size_t yPoints = imageGeom.getNumYCells();
  size_t zPoints = imageGeom.getNumZCells();

  ShapeType tupleShape{totalFeatures};
  ShapeType componentShape{3};

  auto sumPtr = DataStoreUtilities::CreateDataStore<float64>(tupleShape, componentShape, IDataAction::Mode::Execute);
  auto compensationPtr = DataStoreUtilities::CreateDataStore<float64>(tupleShape, componentShape, IDataAction::Mode::Execute);
  auto countPtr = DataStoreUtilities::CreateDataStore<uint64>(tupleShape, componentShape, IDataAction::Mode::Execute);

  Float64AbstractDataStore& sum = *sumPtr.get();
  Float64AbstractDataStore& compensation = *compensationPtr.get();
  UInt64AbstractDataStore& count = *countPtr.get();

  sum.fill(0.0);
  compensation.fill(0.0);
  count.fill(0.0);

  // Create data stores to check if feature IDs are periodic
  componentShape[0] = 2;
  auto rangeXStorePtr = DataStoreUtilities::CreateDataStore<uint64>(tupleShape, componentShape, IDataAction::Mode::Execute);
  auto rangeYStorePtr = DataStoreUtilities::CreateDataStore<uint64>(tupleShape, componentShape, IDataAction::Mode::Execute);
  auto rangeZStorePtr = DataStoreUtilities::CreateDataStore<uint64>(tupleShape, componentShape, IDataAction::Mode::Execute);

  UInt64AbstractDataStore& rangeXStoreRef = *rangeXStorePtr.get();
  UInt64AbstractDataStore& rangeYStoreRef = *rangeYStorePtr.get();
  UInt64AbstractDataStore& rangeZStoreRef = *rangeZStorePtr.get();

  // The first part can be expensive so parallelize the algorithm
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, totalFeatures);
  // This is OFF because we spend more time spinning up threads than actually
  // computing things. Maybe if we were to break the total number of features
  // by the total number of cores/threads and do a ParallelTask Algorithm instead
  // we might see some speedup.
  dataAlg.setParallelizationEnabled(false);
  dataAlg.execute(ComputeFeatureCentroidsImpl1(sum, compensation, count, {xPoints, yPoints, zPoints}, imageGeom, featureIdsStoreRef, rangeXStoreRef, rangeYStoreRef, rangeZStoreRef));

  // Here we are only looping over the number of features so let this just go in serial mode.
  // The count store carries the same voxel count in all three components of a feature; a feature with
  // zero cells keeps its default (0,0,0) centroid.
  for(size_t featureId = 0; featureId < totalFeatures; featureId++)
  {
    auto featureId_idx = static_cast<size_t>(featureId * 3);
    if(count[featureId_idx] > 0)
    {
      centroids[featureId_idx] = static_cast<float>(sum[featureId_idx] / static_cast<double>(count[featureId_idx]));
    }

    featureId_idx++; // featureId * 3 + 1
    if(count[featureId_idx] > 0)
    {
      centroids[featureId_idx] = static_cast<float>(sum[featureId_idx] / static_cast<double>(count[featureId_idx]));
    }

    featureId_idx++; // featureId * 3 + 2
    if(count[featureId_idx] > 0)
    {
      centroids[featureId_idx] = static_cast<float>(sum[featureId_idx] / static_cast<double>(count[featureId_idx]));
    }
  }

  if(m_InputValues->IsPeriodic)
  {
    m_MessageHandler.sendInfoMessage("Checking for periodic data.");
    if(GeometryHelpers::Topology::AdjustCentroidsForPeriodicFaces(imageGeom, rangeXStoreRef, rangeYStoreRef, rangeZStoreRef, centroids))
    {
      m_MessageHandler.sendInfoMessage("ComputeFeatureCentroids found Non-Contiguous Features. Centroids may require additional checks.");
    }
  }

  return {};
}
