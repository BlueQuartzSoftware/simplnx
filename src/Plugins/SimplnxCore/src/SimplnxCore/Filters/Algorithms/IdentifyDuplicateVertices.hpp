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

namespace nx::core
{
namespace detail
{
const std::string k_CopySelectedTriangleData("Copy Selected Triangle Data");
const std::string k_CopyAllTriangleData("Copy All Triangle Data");

const nx::core::ChoicesParameter::Choices k_TriangleDataHandlingChoices = {k_CopySelectedTriangleData, k_CopyAllTriangleData};
const nx::core::ChoicesParameter::ValueType k_CopySelectedTriangleArraysIdx = 0ULL;
const nx::core::ChoicesParameter::ValueType k_CopyAllTriangleArraysIdx = 1ULL;

const std::string k_CopySelectedVertexData("Copy Selected Vertex Data");
const std::string k_CopyAllVertexData("Copy All Vertex Data");

const nx::core::ChoicesParameter::Choices k_VertexDataHandlingChoices = {k_CopySelectedVertexData, k_CopyAllVertexData};
const nx::core::ChoicesParameter::ValueType k_CopySelectedVertexArraysIdx = 0ULL;
const nx::core::ChoicesParameter::ValueType k_CopyAllVertexArraysIdx = 1ULL;
} // namespace detail

struct SIMPLNXCORE_EXPORT IdentifyDuplicateVerticesInputValues
{
  DataPath TargetGeometryPath;
  DataPath DuplicatesMaskPath;
};

/**
 * @class IdentifyDuplicateVertices
 * @brief This class walks the vertices and marks any non-unique vertices with a 1 in the supplied mask.
 * Uses a non-stable sort for detection and flagging, so first instance of a value is not guaranteed to be considered the "unique" one if duplicates are discovered.
 */
class SIMPLNXCORE_EXPORT IdentifyDuplicateVertices
{
public:
  IdentifyDuplicateVertices(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, IdentifyDuplicateVerticesInputValues* inputValues);
  ~IdentifyDuplicateVertices() noexcept = default;

  IdentifyDuplicateVertices(const IdentifyDuplicateVertices&) = delete;
  IdentifyDuplicateVertices(IdentifyDuplicateVertices&&) noexcept = delete;
  IdentifyDuplicateVertices& operator=(const IdentifyDuplicateVertices&) = delete;
  IdentifyDuplicateVertices& operator=(IdentifyDuplicateVertices&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const IdentifyDuplicateVerticesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
