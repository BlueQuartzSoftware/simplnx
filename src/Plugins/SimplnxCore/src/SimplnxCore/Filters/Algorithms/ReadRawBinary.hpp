#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{
inline constexpr int32 k_UnsupportedScalarType = -1070;

struct SIMPLNXCORE_EXPORT ReadRawBinaryInputValues
{
  FileSystemPathParameter::ValueType inputFileValue;
  NumericType scalarTypeValue;
  uint64 numberOfComponentsValue;
  ChoicesParameter::ValueType endianValue;
  uint64 skipHeaderBytesValue;
  DataPath createdAttributeArrayPathValue;
};

class SIMPLNXCORE_EXPORT ReadRawBinary
{
public:
  ReadRawBinary(DataStructure& dataStructure, const ReadRawBinaryInputValues& inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
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
} // namespace nx::core
