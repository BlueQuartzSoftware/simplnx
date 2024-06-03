#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace
{

const std::string k_CopySelectedEdgeData("Copy Selected Edge Data");
const std::string k_CopyAllEdgeData("Copy All Edge Data");

const nx::core::ChoicesParameter::Choices k_EdgeDataHandlingChoices = {k_CopySelectedEdgeData, k_CopyAllEdgeData};
const nx::core::ChoicesParameter::ValueType k_CopySelectedEdgeArraysIdx = 0ULL;
const nx::core::ChoicesParameter::ValueType k_CopyAllEdgeArraysIdx = 1ULL;

const std::string k_CopySelectedVertexData("Copy Selected Vertex Data");
const std::string k_CopyAllVertexData("Copy All Vertex Data");

const nx::core::ChoicesParameter::Choices k_VertexDataHandlingChoices = {k_CopySelectedVertexData, k_CopyAllVertexData};
const nx::core::ChoicesParameter::ValueType k_CopySelectedVertexArraysIdx = 0ULL;
const nx::core::ChoicesParameter::ValueType k_CopyAllVertexArraysIdx = 1ULL;

} // namespace

namespace nx::core
{
struct SIMPLNXCORE_EXPORT RemoveFlaggedEdgesInputValues
{
  DataPath EdgeGeometry;
  DataPath MaskArrayPath;
  DataPath ReducedEdgeGeometry;
  // These variables are associated with the Edge Data Handling
  nx::core::ChoicesParameter::ValueType EdgeDataHandling;
  MultiArraySelectionParameter::ValueType SelectedEdgeData;
  DataPath EdgeAttributeMatrixPath;
  // These variables are associated with the Vertex Data Handling
  nx::core::ChoicesParameter::ValueType VertexDataHandling;
  MultiArraySelectionParameter::ValueType SelectedVertexData;
  DataPath VertexAttributeMatrixPath;
};

/**
 * @class ConditionalSetValueFilter

 */
class SIMPLNXCORE_EXPORT RemoveFlaggedEdges
{
public:
  RemoveFlaggedEdges(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RemoveFlaggedEdgesInputValues* inputValues);
  ~RemoveFlaggedEdges() noexcept = default;

  RemoveFlaggedEdges(const RemoveFlaggedEdges&) = delete;
  RemoveFlaggedEdges(RemoveFlaggedEdges&&) noexcept = delete;
  RemoveFlaggedEdges& operator=(const RemoveFlaggedEdges&) = delete;
  RemoveFlaggedEdges& operator=(RemoveFlaggedEdges&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RemoveFlaggedEdgesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
