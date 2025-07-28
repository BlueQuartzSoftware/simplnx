#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  RemoveFlaggedVerticesInputValues inputValues;
  inputValues.InputVertexGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(input_vertex_geometry_path);
  inputValues.MaskPath = filterArgs.value<ArraySelectionParameter::ValueType>(mask_path);
  inputValues.OutputVertexGeometryPath = filterArgs.value<DataGroupCreationParameter::ValueType>(output_vertex_geometry_path);
  return RemoveFlaggedVertices(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT RemoveFlaggedVerticesInputValues
{
  GeometrySelectionParameter::ValueType InputVertexGeometryPath;
  ArraySelectionParameter::ValueType MaskPath;
  DataGroupCreationParameter::ValueType OutputVertexGeometryPath;
};

/**
 * @class RemoveFlaggedVertices
 * @brief This algorithm implements support code for the RemoveFlaggedVerticesFilter
 */

class SIMPLNXCORE_EXPORT RemoveFlaggedVertices
{
public:
  RemoveFlaggedVertices(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RemoveFlaggedVerticesInputValues* inputValues);
  ~RemoveFlaggedVertices() noexcept;

  RemoveFlaggedVertices(const RemoveFlaggedVertices&) = delete;
  RemoveFlaggedVertices(RemoveFlaggedVertices&&) noexcept = delete;
  RemoveFlaggedVertices& operator=(const RemoveFlaggedVertices&) = delete;
  RemoveFlaggedVertices& operator=(RemoveFlaggedVertices&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const RemoveFlaggedVerticesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
