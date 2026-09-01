#include "NearestPointFuseRegularGrids.hpp"

#include "NearestPointFuseRegularGridsDirect.hpp"
#include "NearestPointFuseRegularGridsScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

NearestPointFuseRegularGrids::NearestPointFuseRegularGrids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                           NearestPointFuseRegularGridsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

NearestPointFuseRegularGrids::~NearestPointFuseRegularGrids() noexcept = default;

const std::atomic_bool& NearestPointFuseRegularGrids::getCancel()
{
  return m_ShouldCancel;
}

Result<> NearestPointFuseRegularGrids::operator()()
{
  auto& refImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ReferenceGeometryPath);
  auto& sampleImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->SamplingGeometryPath);
  auto& sampleAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->SamplingCellAttributeMatrixPath);
  Vec3<float32> sampleRes = sampleImageGeom.getSpacing();

  // Coordinate mapping divides by sampling spacing. Reject zero spacing before dispatch.
  bool resHasZero = std::find(sampleRes.begin(), sampleRes.end(), 0.0f) != std::end(sampleRes) ? true : false;
  if(resHasZero)
  {
    return MakeErrorResult(-5555, fmt::format("A component of the resolution for the Image Geometry associated with DataContainer '{}' is 0. This would result in a division by 0 operation",
                                              m_InputValues->SamplingGeometryPath.toString()));
  }

  std::vector<const IArray*> targets;
  auto sampleVoxelArrays = sampleAM.findAllChildrenOfType<IArray>();
  for(const auto& array : sampleVoxelArrays)
  {
    // The filter creates matching outputs only for numeric and Boolean DataArrays.
    if(array->getArrayType() != IArray::ArrayType::DataArray)
    {
      continue;
    }
    auto& refAMArray = m_DataStructure.getDataRefAs<IArray>(m_InputValues->ReferenceCellAttributeMatrixPath.createChildPath(array->getName()));
    targets.push_back(array.get());
    targets.push_back(&refAMArray);
  }
  return DispatchAlgorithm<NearestPointFuseRegularGridsDirect, NearestPointFuseRegularGridsScanline>(AlgorithmArrayTargets(std::move(targets)), m_DataStructure, m_MessageHandler, m_ShouldCancel,
                                                                                                     m_InputValues);
}
