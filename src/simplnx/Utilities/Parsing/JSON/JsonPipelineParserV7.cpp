#include "JsonPipelineParserV7.h"

using namespace nx::core;

JsonPipelineParserV7::JsonPipelineParserV7(FilterList* filterList)
: IJsonPipelineParser(filterList)
{
}
JsonPipelineParserV7::JsonPipelineParserV7(const JsonPipelineParserV7& other)
: IJsonPipelineParser(other)
{
}
JsonPipelineParserV7::JsonPipelineParserV7(JsonPipelineParserV7&& other) noexcept
: IJsonPipelineParser(std::move(other))
{
}

JsonPipelineParserV7::~JsonPipelineParserV7() = default;
