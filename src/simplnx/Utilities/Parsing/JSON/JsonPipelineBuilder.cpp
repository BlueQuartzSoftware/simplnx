#include "JsonPipelineBuilder.h"

using namespace nx::core;

JsonPipelineBuilder::JsonPipelineBuilder(FilterList* filterList)
: m_FilterList(filterList)
{
}

JsonPipelineBuilder::JsonPipelineBuilder(const JsonPipelineBuilder& other)
: m_FilterList(other.m_FilterList)
{
}

JsonPipelineBuilder::JsonPipelineBuilder(JsonPipelineBuilder&& other) noexcept
: m_FilterList(std::move(other.m_FilterList))
{
}

JsonPipelineBuilder::~JsonPipelineBuilder() = default;
