#include "ComputeFeatureCentroids.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>

namespace
{
// A feature with a per-axis unit-vector resultant below this threshold has its mass spread almost uniformly around the domain.
// This distribution makes the circular mean indeterminate. The algorithm keeps the arithmetic mean.
constexpr double k_DegenerateResultant = 1.0e-6;
} // namespace

using namespace nx::core;

namespace
{
// Computes each feature centroid from the voxel-center coordinates of its cells.
// Kahan compensated summation reduces floating-point round-off for large features.
// m_Sum contains per-component sums. m_Compensation contains the low-order bits lost on the previous add.
// The algorithm calculates the centroid as m_Sum / m_Count.
class ComputeFeatureCentroidsImpl1
{
public:
  ComputeFeatureCentroidsImpl1(Float64AbstractDataStore& sum, Float64AbstractDataStore& compensation, UInt64AbstractDataStore& count, std::array<size_t, 3> dims, const nx::core::ImageGeom& imageGeom,
                               const Int32AbstractDataStore& featureIds, UInt64AbstractDataStore& rangeXStoreRef, UInt64AbstractDataStore& rangeYStoreRef, UInt64AbstractDataStore& rangeZStoreRef,
                               bool isPeriodic, Float64AbstractDataStore* sumCos, Float64AbstractDataStore* sumSin, std::array<double, 3> origin, std::array<double, 3> domainLength)
  : m_Sum(sum)
  , m_Compensation(compensation)
  , m_Count(count)
  , m_Dims(dims)
  , m_ImageGeom(imageGeom)
  , m_FeatureIds(featureIds)
  , m_RangeXStoreRef(rangeXStoreRef)
  , m_RangeYStoreRef(rangeYStoreRef)
  , m_RangeZStoreRef(rangeZStoreRef)
  , m_IsPeriodic(isPeriodic)
  , m_SumCos(sumCos)
  , m_SumSin(sumSin)
  , m_Origin(origin)
  , m_DomainLength(domainLength)
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

          // For periodic runs, also accumulate the unit vector of each cell center's angular position
          // around the domain on each axis. The per-feature (cos, sin) sums produce a minimum-image
          // centroid at finalize for features that wrap the boundary.
          if(m_IsPeriodic)
          {
            const nx::core::Point3Dd center = {voxel_center[0], voxel_center[1], voxel_center[2]};
            for(size_t axis = 0; axis < 3; axis++)
            {
              const double phase = Constants::k_2Pi<double> * (center[axis] - m_Origin[axis]) / m_DomainLength[axis];
              const size_t axisIdx = featureId * 3ULL + axis;
              (*m_SumCos)[axisIdx] = m_SumCos->getValue(axisIdx) + std::cos(phase);
              (*m_SumSin)[axisIdx] = m_SumSin->getValue(axisIdx) + std::sin(phase);
            }
          }
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
  bool m_IsPeriodic = false;
  Float64AbstractDataStore* m_SumCos = nullptr;
  Float64AbstractDataStore* m_SumSin = nullptr;
  std::array<double, 3> m_Origin = {0.0, 0.0, 0.0};
  std::array<double, 3> m_DomainLength = {0.0, 0.0, 0.0};
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

  std::shared_ptr<Float64AbstractDataStore> sumCosPtr;
  std::shared_ptr<Float64AbstractDataStore> sumSinPtr;
  if(m_InputValues->IsPeriodic)
  {
    sumCosPtr = DataStoreUtilities::CreateDataStore<float64>(tupleShape, componentShape, IDataAction::Mode::Execute);
    sumSinPtr = DataStoreUtilities::CreateDataStore<float64>(tupleShape, componentShape, IDataAction::Mode::Execute);
    sumCosPtr->fill(0.0);
    sumSinPtr->fill(0.0);
  }

  const auto geomOrigin = imageGeom.getOrigin();
  const auto geomSpacing = imageGeom.getSpacing();
  const std::array<double, 3> origin = {static_cast<double>(geomOrigin[0]), static_cast<double>(geomOrigin[1]), static_cast<double>(geomOrigin[2])};
  // Periodic domain length on each axis = number of cells * spacing (the full physical extent).
  const std::array<double, 3> domainLength = {static_cast<double>(xPoints) * geomSpacing[0], static_cast<double>(yPoints) * geomSpacing[1], static_cast<double>(zPoints) * geomSpacing[2]};

  // Create data stores to check if feature IDs are periodic
  componentShape[0] = 2;
  auto rangeXStorePtr = DataStoreUtilities::CreateDataStore<uint64>(tupleShape, componentShape, IDataAction::Mode::Execute);
  auto rangeYStorePtr = DataStoreUtilities::CreateDataStore<uint64>(tupleShape, componentShape, IDataAction::Mode::Execute);
  auto rangeZStorePtr = DataStoreUtilities::CreateDataStore<uint64>(tupleShape, componentShape, IDataAction::Mode::Execute);

  UInt64AbstractDataStore& rangeXStoreRef = *rangeXStorePtr.get();
  UInt64AbstractDataStore& rangeYStoreRef = *rangeYStorePtr.get();
  UInt64AbstractDataStore& rangeZStoreRef = *rangeZStorePtr.get();

  // This phase scans all cells. Parallelization is disabled because thread startup currently costs more than serial execution.
  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, totalFeatures);
  dataAlg.setParallelizationEnabled(false);
  dataAlg.execute(ComputeFeatureCentroidsImpl1(sum, compensation, count, {xPoints, yPoints, zPoints}, imageGeom, featureIdsStoreRef, rangeXStoreRef, rangeYStoreRef, rangeZStoreRef,
                                               m_InputValues->IsPeriodic, sumCosPtr.get(), sumSinPtr.get(), origin, domainLength));

  // The count store has the same voxel count in all three components. A feature with zero cells keeps its default (0, 0, 0) centroid.
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
    m_MessageHandler({IFilter::Message::Type::Info, "Checking for periodic data."});

    // When a feature spans the full extent on an axis, its arithmetic centroid is in the empty middle of the wrapped feature.
    // Replace that component with the circular mean from the per-feature unit-vector sums.
    // If the resultant is near zero, the feature mass is almost uniform and the algorithm keeps the arithmetic mean.
    const std::array<UInt64AbstractDataStore*, 3> rangeStores = {&rangeXStoreRef, &rangeYStoreRef, &rangeZStoreRef};
    const std::array<size_t, 3> dims = {xPoints, yPoints, zPoints};
    bool anyAdjusted = false;
    for(size_t featureId = 0; featureId < totalFeatures; featureId++)
    {
      for(size_t axis = 0; axis < 3; axis++)
      {
        const size_t axisIdx = featureId * 3 + axis;
        if(count[axisIdx] == 0)
        {
          continue;
        }
        const UInt64AbstractDataStore& rangeStore = *rangeStores[axis];
        const bool spansExtent = (rangeStore.getValue(featureId * 2 + 0) == 0 && rangeStore.getValue(featureId * 2 + 1) == dims[axis] - 1);
        if(!spansExtent)
        {
          continue;
        }
        const double sumCosValue = sumCosPtr->getValue(axisIdx);
        const double sumSinValue = sumSinPtr->getValue(axisIdx);
        const double resultant = std::sqrt(sumCosValue * sumCosValue + sumSinValue * sumSinValue) / static_cast<double>(count[axisIdx]);
        if(resultant < k_DegenerateResultant)
        {
          continue;
        }
        double phase = std::atan2(sumSinValue, sumCosValue);
        if(phase < 0.0)
        {
          phase += Constants::k_2Pi<double>;
        }
        centroids[axisIdx] = static_cast<float>(origin[axis] + (phase / Constants::k_2Pi<double>)*domainLength[axis]);
        anyAdjusted = true;
      }
    }
    if(anyAdjusted)
    {
      m_MessageHandler({IFilter::Message::Type::Info, "ComputeFeatureCentroids adjusted centroids of features that wrap the periodic boundary."});
    }
  }

  return {};
}
