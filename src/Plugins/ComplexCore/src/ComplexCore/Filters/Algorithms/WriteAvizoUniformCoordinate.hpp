#pragma once

#include "ComplexCore/ComplexCore_export.hpp"
#include "ComplexCore/utils/AvizoWriter.hpp"

namespace complex
{
/**
 * @class WriteAvizoUniformCoordinate
 * @brief This filter writes out a native Avizo Uniform Coordinate data file.
 */

class COMPLEXCORE_EXPORT WriteAvizoUniformCoordinate : public AvizoWriter
{
public:
  WriteAvizoUniformCoordinate(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AvizoWriterInputValues* inputValues);
  ~WriteAvizoUniformCoordinate() noexcept override;

  WriteAvizoUniformCoordinate(const WriteAvizoUniformCoordinate&) = delete;
  WriteAvizoUniformCoordinate(WriteAvizoUniformCoordinate&&) noexcept = delete;
  WriteAvizoUniformCoordinate& operator=(const WriteAvizoUniformCoordinate&) = delete;
  WriteAvizoUniformCoordinate& operator=(WriteAvizoUniformCoordinate&&) noexcept = delete;

  Result<> operator()();

protected:
  Result<> generateHeader(FILE* outputFile) const override;
  Result<> writeData(FILE* outputFile) const override;
};

} // namespace complex
