#include "JsonPipelineParserV6.h"

using namespace nx::core;

JsonPipelineParserV6::JsonPipelineParserV6(FilterList* filterList)
: IJsonPipelineParser(filterList)
{
}
JsonPipelineParserV6::JsonPipelineParserV6(const JsonPipelineParserV6& other)
: IJsonPipelineParser(other)
{
}
JsonPipelineParserV6::JsonPipelineParserV6(JsonPipelineParserV6&& other) noexcept
: IJsonPipelineParser(std::move(other))
{
}

JsonPipelineParserV6::~JsonPipelineParserV6() = default;
