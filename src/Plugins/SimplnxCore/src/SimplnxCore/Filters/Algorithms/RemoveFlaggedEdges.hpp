#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace
{

const std::string k_Ignore("Ignore Edge Data");
const std::string k_CopyAll("Copy All Edge Data");
const std::string k_CopySelected("Copy Selected Edge Data");

const nx::core::ChoicesParameter::Choices k_ArrayHandlingChoices = {k_Ignore, k_CopySelected, k_CopyAll};

const nx::core::ChoicesParameter::ValueType k_IgnoreArraysIdx = 0ULL;
const nx::core::ChoicesParameter::ValueType k_CopySelectedArraysIdx = 1ULL;
const nx::core::ChoicesParameter::ValueType k_CopyAllArraysIdx = 2ULL;

} // namespace

namespace nx::core
{
struct SIMPLNXCORE_EXPORT RemoveFlaggedEdgesInputValues
{
  DataPath EdgeGeometry;
  DataPath MaskArrayPath;
  DataPath ReducedEdgeGeometry;
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
