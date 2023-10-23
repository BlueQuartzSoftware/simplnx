#pragma once

#include "ComplexCore/ComplexCore_export.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

namespace complex
{
inline constexpr int32 k_UnsupportedScalarType = -1070;

struct COMPLEXCORE_EXPORT ReadRawBinaryInputValues
{
  FileSystemPathParameter::ValueType inputFileValue;
  NumericType scalarTypeValue;
  uint64 numberOfComponentsValue;
  ChoicesParameter::ValueType endianValue;
  uint64 skipHeaderBytesValue;
  DataPath createdAttributeArrayPathValue;
};

class COMPLEXCORE_EXPORT ReadRawBinary
{
public:
  ReadRawBinary(DataStructure& data, const ReadRawBinaryInputValues& inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~ReadRawBinary() noexcept;

  ReadRawBinary(const ReadRawBinary&) = delete;
  ReadRawBinary(ReadRawBinary&&) noexcept = delete;
  ReadRawBinary& operator=(const ReadRawBinary&) = delete;
  ReadRawBinary& operator=(ReadRawBinary&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadRawBinaryInputValues& m_InputValues;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  Result<> execute();
};
} // namespace complex
