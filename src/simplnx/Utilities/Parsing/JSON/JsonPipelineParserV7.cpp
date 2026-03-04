#include "JsonPipelineParserV7.hpp"

using namespace nx::core;

JsonPipelineParserV7::JsonPipelineParserV7(FilterList* filterList)
: AbstractJsonPipelineParser(filterList)
{
}
JsonPipelineParserV7::JsonPipelineParserV7(const JsonPipelineParserV7& other)
: AbstractJsonPipelineParser(other)
{
}
JsonPipelineParserV7::JsonPipelineParserV7(JsonPipelineParserV7&& other) noexcept
: AbstractJsonPipelineParser(std::move(other))
{
}

JsonPipelineParserV7::~JsonPipelineParserV7() = default;
