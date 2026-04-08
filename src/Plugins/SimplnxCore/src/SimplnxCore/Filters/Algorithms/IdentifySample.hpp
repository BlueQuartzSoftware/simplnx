#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT IdentifySampleInputValues
{
  BoolParameter::ValueType FillHoles;
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  ArraySelectionParameter::ValueType MaskArrayPath;
  BoolParameter::ValueType SliceBySlice;
  ChoicesParameter::ValueType SliceBySlicePlaneIndex;
};

/**
 * @class IdentifySample
 * @brief This algorithm implements support code for the IdentifySampleFilter
 */

class SIMPLNXCORE_EXPORT IdentifySample
{
public:
  IdentifySample(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, IdentifySampleInputValues* inputValues);
  ~IdentifySample() noexcept;

  IdentifySample(const IdentifySample&) = delete;
  IdentifySample(IdentifySample&&) noexcept = delete;
  IdentifySample& operator=(const IdentifySample&) = delete;
  IdentifySample& operator=(IdentifySample&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IdentifySampleInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
