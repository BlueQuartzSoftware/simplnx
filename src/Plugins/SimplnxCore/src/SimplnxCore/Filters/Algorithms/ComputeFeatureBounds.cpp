#include "ComputeFeatureBounds.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
std::pair<Point3Df, Point3Df> CalculateFeatureBounds(int32 activeFeature, const IGeometry::SharedVertexList::store_type& verts, const Int32AbstractDataStore& featureIds)
{
  float32 minX = std::numeric_limits<float32>::lowest();
  float32 minY = std::numeric_limits<float32>::lowest();
  float32 minZ = std::numeric_limits<float32>::lowest();

  float32 maxX = std::numeric_limits<float32>::max();
  float32 maxY = std::numeric_limits<float32>::max();
  float32 maxZ = std::numeric_limits<float32>::max();

  for(usize i = 0; i < verts.getNumberOfTuples(); i++)
  {
    if(featureIds[i] == activeFeature)
    {
      float32 xVal = verts[(i * 3) + 0];
      float32 yVal = verts[(i * 3) + 1];
      float32 zVal = verts[(i * 3) + 2];

      minX = std::min(minX, xVal);
      minY = std::min(minY, yVal);
      minZ = std::min(minZ, zVal);

      maxX = std::max(maxX, xVal);
      maxY = std::max(maxY, yVal);
      maxZ = std::max(maxZ, zVal);
    }
  }

  return std::make_pair(Point3Df(minX, minY, minZ), Point3Df(maxX, maxY, maxZ));
}

class ComputeSplitBoundsImpl
{
public:
  ComputeSplitBoundsImpl(const IGeometry::SharedVertexList::store_type& verts, const Int32AbstractDataStore& featureIds, Float32AbstractDataStore& minBounds, Float32AbstractDataStore& maxBounds)
  : m_Verts(verts)
  , m_FeatureIds(featureIds)
  , m_MinBounds(minBounds)
  , m_MaxBounds(maxBounds)
  {
  }
  ~ComputeSplitBoundsImpl() = default;

  // -----------------------------------------------------------------------------
  void compute(usize start, usize end) const
  {
    for(usize feature = start; feature < end; feature++)
    {
      std::pair<Point3Df, Point3Df> minMax = CalculateFeatureBounds(feature, m_Verts, m_FeatureIds);

      // Set Min
      m_MinBounds[(feature * 3) + 0] = minMax.first.getX();
      m_MinBounds[(feature * 3) + 1] = minMax.first.getY();
      m_MinBounds[(feature * 3) + 2] = minMax.first.getZ();

      // Set Max
      m_MaxBounds[(feature * 3) + 0] = minMax.second.getX();
      m_MaxBounds[(feature * 3) + 1] = minMax.second.getY();
      m_MaxBounds[(feature * 3) + 2] = minMax.second.getZ();
    }
  }

  // -----------------------------------------------------------------------------
  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  const IGeometry::SharedVertexList::store_type& m_Verts;
  const Int32AbstractDataStore& m_FeatureIds;
  Float32AbstractDataStore& m_MinBounds;
  Float32AbstractDataStore& m_MaxBounds;
};

class ComputeUnifiedBoundsImpl
{
public:
  ComputeUnifiedBoundsImpl(const IGeometry::SharedVertexList::store_type& verts, const Int32AbstractDataStore& featureIds, Float32AbstractDataStore& unifiedBounds)
  : m_Verts(verts)
  , m_FeatureIds(featureIds)
  , m_UnifiedBounds(unifiedBounds)
  {
  }
  ~ComputeUnifiedBoundsImpl() = default;

  // -----------------------------------------------------------------------------
  void compute(usize start, usize end) const
  {
    for(usize feature = start; feature < end; feature++)
    {
      std::pair<Point3Df, Point3Df> minMax = CalculateFeatureBounds(feature, m_Verts, m_FeatureIds);

      // Set Min
      m_UnifiedBounds[(feature * 6) + 0] = minMax.first.getX();
      m_UnifiedBounds[(feature * 6) + 1] = minMax.first.getY();
      m_UnifiedBounds[(feature * 6) + 2] = minMax.first.getZ();

      // Set Max
      m_UnifiedBounds[(feature * 6) + 3] = minMax.second.getX();
      m_UnifiedBounds[(feature * 6) + 4] = minMax.second.getY();
      m_UnifiedBounds[(feature * 6) + 5] = minMax.second.getZ();
    }
  }

  // -----------------------------------------------------------------------------
  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  const IGeometry::SharedVertexList::store_type& m_Verts;
  const Int32AbstractDataStore& m_FeatureIds;
  Float32AbstractDataStore& m_UnifiedBounds;
};
} // namespace

// -----------------------------------------------------------------------------
ComputeFeatureBounds::ComputeFeatureBounds(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureBoundsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureBounds::~ComputeFeatureBounds() noexcept = default;

// -----------------------------------------------------------------------------
void ComputeFeatureBounds::updateProgress(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeFeatureBounds::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeFeatureBounds::operator()()
{
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  const auto& featureAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->FeatureAMPath);

  const int32 numFeatures = (*std::max_element(featureIds.cbegin(), featureIds.cend())) + 1;
  if(numFeatures > featureAM.getNumTuples())
  {
    return MakeErrorResult(-89471, fmt::format("{} Attribute Matrix size ({}) doesn't align with number of features ({}) in {} array", m_InputValues->FeatureAMPath.getTargetName(),
                                               featureAM.getNumTuples(), numFeatures, m_InputValues->FeatureIdsArrayPath.getTargetName()));
  }

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, numFeatures);

  const auto& verts = m_DataStructure.getDataRefAs<IGeometry::SharedVertexList>(m_InputValues->VertsArrayPath).getDataStoreRef();
  switch(static_cast<OutputDataType>(m_InputValues->OutputType))
  {
  case OutputDataType::Split: {
    auto& minArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->MinArrayPath).getDataStoreRef();
    auto& maxArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->MaxArrayPath).getDataStoreRef();
    dataAlg.execute(ComputeSplitBoundsImpl(verts, featureIds, minArray, maxArray));
    break;
  }
  case OutputDataType::Unified: {
    auto& unifiedArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->UnifiedArrayPath).getDataStoreRef();
    dataAlg.execute(ComputeUnifiedBoundsImpl(verts, featureIds, unifiedArray));
    break;
  }
  }

  return {};
}
