#pragma once

#include "ComplexCore/ComplexCore_export.hpp"
#include "ComplexCore/utils/AvizoWriter.hpp"

namespace complex
{
/**
 * @class AvizoRectilinearCoordinateWriter
 * @brief This filter writes out a native Avizo Rectilinear Coordinate data file.
 */

class COMPLEXCORE_EXPORT AvizoRectilinearCoordinateWriter : public AvizoWriter
{
public:
  AvizoRectilinearCoordinateWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AvizoWriterInputValues* inputValues);
  ~AvizoRectilinearCoordinateWriter() noexcept override;

  AvizoRectilinearCoordinateWriter(const AvizoRectilinearCoordinateWriter&) = delete;
  AvizoRectilinearCoordinateWriter(AvizoRectilinearCoordinateWriter&&) noexcept = delete;
  AvizoRectilinearCoordinateWriter& operator=(const AvizoRectilinearCoordinateWriter&) = delete;
  AvizoRectilinearCoordinateWriter& operator=(AvizoRectilinearCoordinateWriter&&) noexcept = delete;

  Result<> operator()();

protected:
  Result<> generateHeader(FILE* outputFile) const override;
  Result<> writeData(FILE* outputFile) const override;
};

} // namespace complex
