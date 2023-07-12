#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

namespace complex
{
struct ITKIMAGEPROCESSING_EXPORT BoundsType
{
  fs::path Filename;
  SizeVec3 Dims;
  FloatVec3 Origin;
  FloatVec3 Spacing;
  usize Row;
  usize Col;
  DataPath ImageDataProxy;
  IGeometry::LengthUnit LengthUnit;
};

struct ITKIMAGEPROCESSING_EXPORT FijiCache
{
  fs::path inputFile;
  std::vector<BoundsType> bounds;
  usize vertexAttrMatTupleCount;
  usize cellAttrMatTupleCount;
  fs::file_time_type timeStamp;

  void flush()
  {
    inputFile.clear();
    bounds.clear();
    timeStamp = fs::file_time_type();
  }
};

struct ITKIMAGEPROCESSING_EXPORT ITKImportFijiMontageInputValues
{
  bool FindHistogram;
};

/**
 * @class ITKImportImageStack
 * @brief This filter will ....
 */
class ITKIMAGEPROCESSING_EXPORT ITKImportFijiMontage
{
public:
  ITKImportFijiMontage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ITKImportFijiMontageInputValues* inputValues);
  ~ITKImportFijiMontage() noexcept = default;

  ITKImportFijiMontage(const ITKImportFijiMontage&) = delete;
  ITKImportFijiMontage(ITKImportFijiMontage&&) noexcept = delete;
  ITKImportFijiMontage& operator=(const ITKImportFijiMontage&) = delete;
  ITKImportFijiMontage& operator=(ITKImportFijiMontage&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  FijiCache& getCache();

private:
  DataStructure& m_DataStructure;
  const ITKImportFijiMontageInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  FijiCache m_Cache = {};
};
} // namespace complex
