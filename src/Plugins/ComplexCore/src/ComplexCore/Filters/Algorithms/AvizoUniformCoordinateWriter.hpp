#pragma once

#include "ComplexCore/ComplexCore_export.hpp"
#include "ComplexCore/utils/AvizoWriter.hpp"

namespace complex
{
/**
 * @class AvizoUniformCoordinateWriter
 * @brief This filter writes out a native Avizo Uniform Coordinate data file.
 */

class COMPLEXCORE_EXPORT AvizoUniformCoordinateWriter : public AvizoWriter
{
public:
  AvizoUniformCoordinateWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AvizoWriterInputValues* inputValues);
  ~AvizoUniformCoordinateWriter() noexcept override;

  AvizoUniformCoordinateWriter(const AvizoUniformCoordinateWriter&) = delete;
  AvizoUniformCoordinateWriter(AvizoUniformCoordinateWriter&&) noexcept = delete;
  AvizoUniformCoordinateWriter& operator=(const AvizoUniformCoordinateWriter&) = delete;
  AvizoUniformCoordinateWriter& operator=(AvizoUniformCoordinateWriter&&) noexcept = delete;

  Result<> operator()();

protected:
  Result<> generateHeader(FILE* outputFile) const override;
  Result<> writeData(FILE* outputFile) const override;
};

} // namespace complex
