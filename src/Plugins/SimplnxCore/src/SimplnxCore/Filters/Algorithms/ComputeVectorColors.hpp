#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeVectorColorsInputValues
{
  bool UseMask;
  DataPath VectorsArrayPath;
  DataPath MaskArrayPath;
  DataPath CellVectorColorsArrayPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ComputeVectorColors
{
public:
  ComputeVectorColors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeVectorColorsInputValues* inputValues);
  ~ComputeVectorColors() noexcept;

  ComputeVectorColors(const ComputeVectorColors&) = delete;
  ComputeVectorColors(ComputeVectorColors&&) noexcept = delete;
  ComputeVectorColors& operator=(const ComputeVectorColors&) = delete;
  ComputeVectorColors& operator=(ComputeVectorColors&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeVectorColorsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
