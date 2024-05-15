#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT ComputeColorTableInputValues
{
  std::string PresetName;
  DataPath SelectedDataArrayPath;
  DataPath RgbArrayPath;
  bool UseMask;
  DataPath MaskArrayPath;
  std::vector<uint8> InvalidColor;
};

class SIMPLNXCORE_EXPORT ComputeColorTable
{
public:
  ComputeColorTable(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ComputeColorTableInputValues* inputValues);
  ~ComputeColorTable() noexcept;

  ComputeColorTable(const ComputeColorTable&) = delete;
  ComputeColorTable(ComputeColorTable&&) noexcept = delete;
  ComputeColorTable& operator=(const ComputeColorTable&) = delete;
  ComputeColorTable& operator=(ComputeColorTable&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeColorTableInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
