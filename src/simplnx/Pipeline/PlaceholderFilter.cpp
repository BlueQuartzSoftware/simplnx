#include "PlaceholderFilter.hpp"

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
}

PlaceholderFilter::~PlaceholderFilter() noexcept = default;

bool PlaceholderFilter::preflight(DataStructure& data, const std::atomic_bool& shouldCancel)
{
  return true;
}

bool PlaceholderFilter::preflight(DataStructure& data, RenamedPaths& renamedPaths, const std::atomic_bool& shouldCancel, bool allowRenaming)
{
  return true;
}

bool PlaceholderFilter::execute(DataStructure& data, const std::atomic_bool& shouldCancel)
{
  return true;
}

std::unique_ptr<AbstractPipelineNode> PlaceholderFilter::deepCopy() const
{
  return std::make_unique<PlaceholderFilter>(m_Json);
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
