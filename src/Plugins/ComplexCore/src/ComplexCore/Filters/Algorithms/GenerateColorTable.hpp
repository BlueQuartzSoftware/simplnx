#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GenerateColorTableParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT GenerateColorTableInputValues
{
  nlohmann::json SelectedPreset;
  DataPath SelectedDataArrayPath;
  DataPath RgbArrayPath;
  bool UseMask;
  DataPath MaskArrayPath;
  std::vector<uint8> InvalidColor;
};

class COMPLEXCORE_EXPORT GenerateColorTable
{
public:
  GenerateColorTable(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, GenerateColorTableInputValues* inputValues);
  ~GenerateColorTable() noexcept;

  GenerateColorTable(const GenerateColorTable&) = delete;
  GenerateColorTable(GenerateColorTable&&) noexcept = delete;
  GenerateColorTable& operator=(const GenerateColorTable&) = delete;
  GenerateColorTable& operator=(GenerateColorTable&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GenerateColorTableInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
