#pragma once

#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/IO/Generic/IDataIOManager.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace nx::core::UnitTest
{
// Advertises numeric-store capability for metadata planning without allocating storage.
class PlaceholderPlanManager : public IDataIOManager
{
public:
  explicit PlaceholderPlanManager(std::string format)
  : m_Format(std::move(format))
  {
    addDataStoreCreationFnc(m_Format, [](DataType, const ShapeType&, const ShapeType&, const std::optional<ShapeType>&, DataStoreInitializationMode) -> std::unique_ptr<IDataStore> {
      throw std::runtime_error("The metadata-only import must not invoke the value-store factory");
    });
  }

  ~PlaceholderPlanManager() noexcept override = default;
  PlaceholderPlanManager(const PlaceholderPlanManager&) = delete;
  PlaceholderPlanManager(PlaceholderPlanManager&&) = delete;
  PlaceholderPlanManager& operator=(const PlaceholderPlanManager&) = delete;
  PlaceholderPlanManager& operator=(PlaceholderPlanManager&&) = delete;

  std::string formatName() const override
  {
    return m_Format;
  }

private:
  std::string m_Format;
};

// Registration replaces matching names, so retain any manager already provided by the application.
[[nodiscard]] inline Result<> RegisterPlaceholderPlanManager(DataIOCollection& collection, const std::string& format)
{
  if(collection.getManager(format) != nullptr)
  {
    return {};
  }
  return collection.addIOManager(std::make_shared<PlaceholderPlanManager>(format));
}
} // namespace nx::core::UnitTest
