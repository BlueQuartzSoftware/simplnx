#pragma once

#include "ComplexCore/ComplexCore_export.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

namespace complex
{
inline constexpr int32 k_UnsupportedScalarType = -1070;

struct COMPLEXCORE_EXPORT RawBinaryReaderInputValues
{
  FileSystemPathParameter::ValueType inputFileValue;
  NumericType scalarTypeValue;
  uint64 numberOfComponentsValue;
  ChoicesParameter::ValueType endianValue;
  uint64 skipHeaderBytesValue;
  DataPath createdAttributeArrayPathValue;
};

class COMPLEXCORE_EXPORT RawBinaryReader
{
public:
  RawBinaryReader(DataStructure& data, const RawBinaryReaderInputValues& inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~RawBinaryReader() noexcept;

  RawBinaryReader(const RawBinaryReader&) = delete;
  RawBinaryReader(RawBinaryReader&&) noexcept = delete;
  RawBinaryReader& operator=(const RawBinaryReader&) = delete;
  RawBinaryReader& operator=(RawBinaryReader&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const RawBinaryReaderInputValues& m_InputValues;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  Result<> execute();
};
} // namespace complex
