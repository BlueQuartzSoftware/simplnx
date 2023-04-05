#include "NearestPointFuseRegularGrids.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/ParallelAlgorithmUtilities.hpp"
#include "complex/Utilities/ParallelTaskAlgorithm.hpp"

using namespace complex;

namespace
{
template <typename T>
class CopyFromIndicesListImpl
{
public:
  CopyFromIndicesListImpl(const IDataArray& sourceArray, IDataArray& destArray, const std::vector<std::pair<usize, usize>>& sourceToDest)
  : m_SourceArray(dynamic_cast<const DataArray<T>&>(sourceArray))
  , m_DestArray(dynamic_cast<DataArray<T>&>(destArray))
  , m_IndicesList(sourceToDest)
  {
  }
  ~CopyFromIndicesListImpl() = default;

  CopyFromIndicesListImpl(const CopyFromIndicesListImpl&) = default;
  CopyFromIndicesListImpl(CopyFromIndicesListImpl&&) noexcept = default;
  CopyFromIndicesListImpl& operator=(const CopyFromIndicesListImpl&) = delete;
  CopyFromIndicesListImpl& operator=(CopyFromIndicesListImpl&&) noexcept = delete;

  void operator()() const
  {
    usize numComps = m_SourceArray.getNumberOfComponents();
    for(const auto& [source, dest] : m_IndicesList)
    {
      usize sourceIndex = source * numComps;
      usize destIndex = dest * numComps;
      for(usize i = 0; i < numComps; i++)
      {
        m_DestArray[destIndex + i] = m_SourceArray[destIndex + i];
      }
    }
  }

private:
  const DataArray<T>& m_SourceArray;
  DataArray<T>& m_DestArray;
  const std::vector<std::pair<usize, usize>>& m_IndicesList;
};
} // namespace

// -----------------------------------------------------------------------------
NearestPointFuseRegularGrids::NearestPointFuseRegularGrids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           NearestPointFuseRegularGridsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
NearestPointFuseRegularGrids::~NearestPointFuseRegularGrids() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& NearestPointFuseRegularGrids::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> NearestPointFuseRegularGrids::operator()()
{
  auto& refAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->ReferenceCellAttributeMatrixPath);
  auto& sampleAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->SamplingCellAttributeMatrixPath);

  std::vector<std::pair<usize, usize>> sourceAndDestIndices = {};
  sourceAndDestIndices.reserve(refAM.getNumTuples());

  { // scoped to ensure mem and namespace is cleaner
    auto& refImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ReferenceGeometryPath);
    auto& sampleImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SamplingGeometryPath);

    // Get dimensions and resolutions of two grids
    Vec3<float32> refRes = refImageGeom.getSpacing();
    Vec3<float32> sampleRes = sampleImageGeom.getSpacing();
    Vec3<float32> refOrigin = refImageGeom.getOrigin();
    Vec3<float32> sampleOrigin = sampleImageGeom.getOrigin();
    Vec3<usize> refDims = refImageGeom.getDimensions();
    Vec3<usize> sampleDims = sampleImageGeom.getDimensions();

    // Further down we divide by sampleRes, so here check to make sure that no components of the resolution are 0
    // This would be incredible unusual behavior if it were to occur.
    bool resHasZero = std::find(sampleRes.begin(), sampleRes.end(), 0.0f) != std::end(sampleRes) ? true : false;
    if(resHasZero)
    {
      return MakeErrorResult(-5555, fmt::format("A component of the resolution for the Image Geometry associated with DataContainer '{}' is 0. This would result in a division by 0 operation",
                                                m_InputValues->SamplingGeometryPath.toString()));
    }

    float32 x = 0.0f, y = 0.0f, z = 0.0f;
    usize col = 0, row = 0, plane = 0;
    usize refIndex = 0;
    usize sampleIndex = 0;
    usize planeComp = 0, rowComp = 0;

    for(usize i = 0; i < refDims[2]; i++)
    {
      if(getCancel())
      {
        return {};
      }

      planeComp = i * refDims[0] * refDims[1];
      for(usize j = 0; j < refDims[1]; j++)
      {
        rowComp = j * refDims[0];
        for(usize k = 0; k < refDims[0]; k++)
        {
          x = (k * refRes[0] + refOrigin[0]);
          y = (j * refRes[1] + refOrigin[1]);
          z = (i * refRes[2] + refOrigin[2]);
          if((x - sampleOrigin[0]) < 0)
          {
            continue;
          }
          if((y - sampleOrigin[1]) < 0)
          {
            continue;
          }
          if((z - sampleOrigin[2]) < 0)
          {
            continue;
          }

          col = usize((x - sampleOrigin[0]) / sampleRes[0]);
          row = usize((y - sampleOrigin[1]) / sampleRes[1]);
          plane = usize((z - sampleOrigin[2]) / sampleRes[2]);
          if(col >= sampleDims[0] || row >= sampleDims[1] || plane >= sampleDims[2])
          {
            continue;
          }

          sampleIndex = (plane * sampleDims[0] * sampleDims[1]) + (row * sampleDims[0]) + col;
          refIndex = planeComp + rowComp + k;

          sourceAndDestIndices.emplace_back(sampleIndex, refIndex); // no need for std::move trivial copy
        }
      }
    }
  }

  sourceAndDestIndices.shrink_to_fit();

  // Copy according to  calculated values
  ParallelTaskAlgorithm taskRunner;
  auto sampleVoxelArrays = sampleAM.findAllChildrenOfType<IDataArray>();
  for(const auto& array : sampleVoxelArrays)
  {
    // this path was created in preflight
    DataPath createdArrayPath = m_InputValues->ReferenceCellAttributeMatrixPath.createChildPath(array->getName());
    auto& refAMArray = m_DataStructure.getDataRefAs<IDataArray>(createdArrayPath);

    ExecuteParallelFunction<CopyFromIndicesListImpl>(array->getDataType(), taskRunner, *array, refAMArray, sourceAndDestIndices);
  }
  taskRunner.wait();

  return {};
}
