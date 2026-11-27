#include "RemoveFlaggedVertices.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/SamplingUtils.hpp"

using namespace nx::core;

namespace
{
/**
 * @struct RemoveFlaggedVerticesFunctor
 * @brief Copies retained vertex data and returns the first destination-store error.
 */
struct RemoveFlaggedVerticesFunctor
{
  /**
   * @brief Copies retained tuples to the destination array.
   * @tparam T Specifies the array value type.
   * @param sourceIDataArray Source vertex data.
   * @param destIDataArray Destination vertex data.
   * @param maskCompare Selects removed vertices.
   * @param numVerticesToKeep Destination tuple count.
   * @return The first destination resize error.
   */
  template <class T>
  Result<> operator()(const IDataArray& sourceIDataArray, IDataArray& destIDataArray, const std::unique_ptr<mask_compare_utilities::MaskCompare>& maskCompare, size_t numVerticesToKeep) const
  {
    const auto& sourceDataStore = sourceIDataArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& destinationDataStore = destIDataArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    Result<> resizeResult = destinationDataStore.resizeTuples({numVerticesToKeep});
    if(resizeResult.invalid())
    {
      return resizeResult;
    }

    const usize numInputTuples = sourceDataStore.getNumberOfTuples();
    const usize nComps = sourceDataStore.getNumberOfComponents();
    usize destTupleIndex = 0;
    for(usize inputIndex = 0; inputIndex < numInputTuples; inputIndex++)
    {
      if(!maskCompare->isTrue(inputIndex))
      {
        for(usize compIdx = 0; compIdx < nComps; compIdx++)
        {
          const usize sourceIndex = (nComps * inputIndex) + compIdx;
          const usize destinationIndex = (nComps * destTupleIndex) + compIdx;
          destinationDataStore[destinationIndex] = sourceDataStore[sourceIndex];
        }
        destTupleIndex++;
      }
    }
    return {};
  }
};
} // namespace

// -----------------------------------------------------------------------------
RemoveFlaggedVertices::RemoveFlaggedVertices(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             RemoveFlaggedVerticesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
Result<> RemoveFlaggedVertices::operator()()
{
  const VertexGeom& vertexGeom = m_DataStructure.getDataRefAs<VertexGeom>(m_InputValues->input_vertex_geometry_path);
  const std::string vertexDataName = vertexGeom.getVertexAttributeMatrixDataPath().getTargetName();

  std::unique_ptr<mask_compare_utilities::MaskCompare> maskCompare;
  try
  {
    maskCompare = mask_compare_utilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->mask_path);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->mask_path.toString());
    return MakeErrorResult(-54070, message);
  }

  const size_t numVerticesToKeep = maskCompare->getNumberOfTuples() - maskCompare->countTrueValues(); // We don't need component size since it must be 1
  const size_t numberOfVertices = vertexGeom.getNumberOfVertices();

  const ShapeType tDims = {numVerticesToKeep};

  // Resize the reduced vertex geometry object
  auto& reducedVertexGeom = m_DataStructure.getDataRefAs<VertexGeom>(m_InputValues->output_vertex_geometry_path);
  Result<> resizeResult = reducedVertexGeom.resizeVertexList(numVerticesToKeep);
  if(resizeResult.invalid())
  {
    return resizeResult;
  }
  resizeResult = reducedVertexGeom.getVertexAttributeMatrix()->resizeTuples(tDims);
  if(resizeResult.invalid())
  {
    return resizeResult;
  }

  m_MessageHandler.sendInfoMessage(fmt::format("Copying vertices to reduced geometry"));

  size_t keepIndex = 0;
  // Loop over each vertex and only copy the vertices that were *NOT* flagged for removal
  for(size_t inputVertexIndex = 0; inputVertexIndex < numberOfVertices; inputVertexIndex++)
  {
    // If the mask value == FALSE we are keeping that vertex.
    if(!maskCompare->isTrue(inputVertexIndex))
    {
      reducedVertexGeom.setVertexCoordinate(keepIndex, vertexGeom.getVertexCoordinate(inputVertexIndex));
      keepIndex++;
    }
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  // Now copy the vertex data from the source arrays to the reduced vertex attribute matrix arrays
  const AttributeMatrix* sourceVertexAttrMatPtr = vertexGeom.getVertexAttributeMatrix();
  for(const auto& [identifier, object] : *sourceVertexAttrMatPtr)
  {
    const auto& src = dynamic_cast<const IDataArray&>(*object);

    const DataPath destinationPath = reducedVertexGeom.getVertexAttributeMatrixDataPath().createChildPath(src.getName());

    auto& dest = m_DataStructure.getDataRefAs<IDataArray>(destinationPath);
    m_MessageHandler.sendInfoMessage(fmt::format("Copying source array '{}' to reduced geometry vertex data.", src.getName()));

    Result<> copyResult = ExecuteDataFunction(RemoveFlaggedVerticesFunctor{}, src.getDataType(), src, dest, maskCompare, numVerticesToKeep);
    if(copyResult.invalid())
    {
      return copyResult;
    }
  }

  if(m_ShouldCancel)
  {
    return {};
  }
  if(m_InputValues->clear_feature_attribute_matrix && m_InputValues->renumber_features)
  {
    // Validate all source IDs, including vertices removed by the mask, as Crop Image does.
    const auto& featureIds = m_DataStructure.getDataRefAs<Int32ArrayType>(m_InputValues->vertex_feature_ids_path);
    auto validateResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->feature_attribute_matrix_path, featureIds, false, m_MessageHandler, &m_ShouldCancel);
    if(validateResult.invalid() || m_ShouldCancel)
    {
      return validateResult;
    }
    const DataPath destFeatureAmPath = m_InputValues->output_vertex_geometry_path.createChildPath(m_InputValues->feature_attribute_matrix_path.getTargetName());
    const DataPath destFeatureIdsPath = reducedVertexGeom.getVertexAttributeMatrixDataPath().createChildPath(m_InputValues->vertex_feature_ids_path.getTargetName());
    return sampling::RenumberFeatures(m_DataStructure, destFeatureAmPath, m_InputValues->vertex_feature_ids_path, destFeatureIdsPath, m_MessageHandler, m_ShouldCancel);
  }

  return {};
}
