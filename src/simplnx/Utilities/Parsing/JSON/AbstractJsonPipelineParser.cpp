#include "AbstractJsonPipelineParser.hpp"

using namespace nx::core;

AbstractJsonPipelineParser::AbstractJsonPipelineParser(FilterList* filterList)
: m_FilterList(filterList)
{
}
AbstractJsonPipelineParser::AbstractJsonPipelineParser(const AbstractJsonPipelineParser& other)
: m_FilterList(other.m_FilterList)
{
}
AbstractJsonPipelineParser::AbstractJsonPipelineParser(AbstractJsonPipelineParser&& other) noexcept
: m_FilterList(std::move(other.m_FilterList))
{
}

AbstractJsonPipelineParser::~AbstractJsonPipelineParser() = default;
