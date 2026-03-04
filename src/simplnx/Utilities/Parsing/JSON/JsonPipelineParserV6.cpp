#include "JsonPipelineParserV6.hpp"

using namespace nx::core;

JsonPipelineParserV6::JsonPipelineParserV6(FilterList* filterList)
: AbstractJsonPipelineParser(filterList)
{
}
JsonPipelineParserV6::JsonPipelineParserV6(const JsonPipelineParserV6& other)
: AbstractJsonPipelineParser(other)
{
}
JsonPipelineParserV6::JsonPipelineParserV6(JsonPipelineParserV6&& other) noexcept
: AbstractJsonPipelineParser(std::move(other))
{
}

JsonPipelineParserV6::~JsonPipelineParserV6() = default;
