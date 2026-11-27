#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT CreateColorMapInputValues
{
  std::string preset_name;
  DataPath selected_data_array_path;
  DataPath rgb_array_path;
  bool use_mask{};
  DataPath mask_array_path;
  std::vector<uint8> invalid_color;
};

class SIMPLNXCORE_EXPORT CreateColorMap
{
public:
  CreateColorMap(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, CreateColorMapInputValues* inputValues);
  ~CreateColorMap() noexcept = default;

  CreateColorMap(const CreateColorMap&) = delete;
  CreateColorMap(CreateColorMap&&) noexcept = delete;
  CreateColorMap& operator=(const CreateColorMap&) = delete;
  CreateColorMap& operator=(CreateColorMap&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreateColorMapInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
