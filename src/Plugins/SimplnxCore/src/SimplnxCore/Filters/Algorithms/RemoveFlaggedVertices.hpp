#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT RemoveFlaggedVerticesInputValues
{
  GeometrySelectionParameter::ValueType input_vertex_geometry_path;
  ArraySelectionParameter::ValueType mask_path;
  DataGroupCreationParameter::ValueType output_vertex_geometry_path;
  BoolParameter::ValueType clear_feature_attribute_matrix = false;
  BoolParameter::ValueType renumber_features = false;
  ArraySelectionParameter::ValueType vertex_feature_ids_path;
  AttributeMatrixSelectionParameter::ValueType feature_attribute_matrix_path;
};

/**
 * @class RemoveFlaggedVertices
 * @brief This algorithm implements support code for the RemoveFlaggedVerticesFilter
 */

class SIMPLNXCORE_EXPORT RemoveFlaggedVertices
{
public:
  RemoveFlaggedVertices(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RemoveFlaggedVerticesInputValues* inputValues);
  ~RemoveFlaggedVertices() noexcept = default;

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
