#include "IJsonPipelineParser.h"

using namespace nx::core;

IJsonPipelineParser::IJsonPipelineParser(FilterList* filterList)
: m_FilterList(filterList)
{
}
IJsonPipelineParser::IJsonPipelineParser(const IJsonPipelineParser& other)
: m_FilterList(other.m_FilterList)
{
}
IJsonPipelineParser::IJsonPipelineParser(IJsonPipelineParser&& other) noexcept
: m_FilterList(std::move(other.m_FilterList))
{
}

IJsonPipelineParser::~IJsonPipelineParser() = default;
