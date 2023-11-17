#pragma once

#include "ComplexCore/ComplexCore_export.hpp"
#include "ComplexCore/utils/AvizoWriter.hpp"

namespace complex
{
/**
 * @class WriteAvizoRectilinearCoordinate
 * @brief This filter writes out a native Avizo Rectilinear Coordinate data file.
 */

class COMPLEXCORE_EXPORT WriteAvizoRectilinearCoordinate : public AvizoWriter
{
public:
  WriteAvizoRectilinearCoordinate(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AvizoWriterInputValues* inputValues);
  ~WriteAvizoRectilinearCoordinate() noexcept override;

  WriteAvizoRectilinearCoordinate(const WriteAvizoRectilinearCoordinate&) = delete;
  WriteAvizoRectilinearCoordinate(WriteAvizoRectilinearCoordinate&&) noexcept = delete;
  WriteAvizoRectilinearCoordinate& operator=(const WriteAvizoRectilinearCoordinate&) = delete;
  WriteAvizoRectilinearCoordinate& operator=(WriteAvizoRectilinearCoordinate&&) noexcept = delete;

  Result<> operator()();

protected:
  Result<> generateHeader(FILE* outputFile) const override;
  Result<> writeData(FILE* outputFile) const override;
};

} // namespace complex