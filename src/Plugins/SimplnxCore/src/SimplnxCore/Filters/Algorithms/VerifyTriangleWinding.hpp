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

struct SIMPLNXCORE_EXPORT VerifyTriangleWindingInputValues
{
  DataPath TargetGeometryPath;
  DataPath FaceLabelsPath;
};

/**
 * @class ConditionalSetValueFilter

 */
class SIMPLNXCORE_EXPORT VerifyTriangleWinding
{
public:
  VerifyTriangleWinding(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, VerifyTriangleWindingInputValues* inputValues);
  ~VerifyTriangleWinding() noexcept = default;

  VerifyTriangleWinding(const VerifyTriangleWinding&) = delete;
  VerifyTriangleWinding(VerifyTriangleWinding&&) noexcept = delete;
  VerifyTriangleWinding& operator=(const VerifyTriangleWinding&) = delete;
  VerifyTriangleWinding& operator=(VerifyTriangleWinding&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const VerifyTriangleWindingInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
