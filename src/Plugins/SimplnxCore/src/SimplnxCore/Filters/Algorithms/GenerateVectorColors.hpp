#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT GenerateVectorColorsInputValues
{
  bool UseMask;
  DataPath VectorsArrayPath;
  DataPath MaskArrayPath;
  DataPath CellVectorColorsArrayPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT GenerateVectorColors
{
public:
  GenerateVectorColors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GenerateVectorColorsInputValues* inputValues);
  ~GenerateVectorColors() noexcept;

  GenerateVectorColors(const GenerateVectorColors&) = delete;
  GenerateVectorColors(GenerateVectorColors&&) noexcept = delete;
  GenerateVectorColors& operator=(const GenerateVectorColors&) = delete;
  GenerateVectorColors& operator=(GenerateVectorColors&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GenerateVectorColorsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
