#include "PlaceholderFilter.hpp"

#include <memory>

using namespace nx::core;

std::unique_ptr<PlaceholderFilter> PlaceholderFilter::Create(nlohmann::json json)
{
  return std::make_unique<PlaceholderFilter>(std::move(json));
}

PlaceholderFilter::PlaceholderFilter() = default;

PlaceholderFilter::PlaceholderFilter(nlohmann::json json)
: AbstractPipelineFilter()
, m_Json(std::move(json))
{
  // Raw fallback JSON can contain an invalid flag; keep that payload without rejecting the placeholder.
  const auto disabledIter = m_Json.find("isDisabled");
  if(disabledIter != m_Json.end() && disabledIter->is_boolean())
  {
    setDisabled(ReadDisabledState(m_Json));
  }
}

PlaceholderFilter::~PlaceholderFilter() noexcept = default;

bool PlaceholderFilter::preflight(DataStructure& dataStructure, const std::atomic_bool& shouldCancel)
{
  return true;
}

bool PlaceholderFilter::preflight(DataStructure& dataStructure, RenamedPaths& renamedPaths, const std::atomic_bool& shouldCancel, bool allowRenaming)
{
  return true;
}

bool PlaceholderFilter::execute(DataStructure& dataStructure, const std::atomic_bool& shouldCancel)
{
  return true;
}

std::unique_ptr<AbstractPipelineNode> PlaceholderFilter::deepCopy() const
{
  auto placeholderFilterPtr = std::make_unique<PlaceholderFilter>(m_Json);
  placeholderFilterPtr->setDisabled(isDisabled());
  return placeholderFilterPtr;
}

nlohmann::json PlaceholderFilter::toJsonImpl() const
{
  return m_Json;
}

std::string PlaceholderFilter::getName() const
{
  return "Placeholder Filter";
}

const nlohmann::json& PlaceholderFilter::getJson() const
{
  return m_Json;
}

AbstractPipelineFilter::FilterType PlaceholderFilter::getFilterType() const
{
  return FilterType::Placeholder;
}
