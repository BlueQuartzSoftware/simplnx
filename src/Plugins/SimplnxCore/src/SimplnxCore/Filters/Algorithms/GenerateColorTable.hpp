#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/util/ColorTable.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT GenerateColorTableInputValues
{
  std::string PresetName;
  DataPath SelectedDataArrayPath;
  DataPath RgbArrayPath;
  bool UseMask;
  DataPath MaskArrayPath;
  std::vector<uint8> InvalidColor;
};

class SIMPLNXCORE_EXPORT GenerateColorTable
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
  ColorTable m_ColorTable = {};
};
} // namespace nx::core
