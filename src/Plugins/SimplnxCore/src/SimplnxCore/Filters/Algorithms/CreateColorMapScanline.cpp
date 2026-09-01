#include "CreateColorMapScanline.hpp"

#include "CreateColorMap.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ColorTableUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <memory>

using namespace nx::core;

namespace
{
// Scalar and RGB buffers process 65,536 tuples per pass.
constexpr usize k_ChunkTuples = 65536;
constexpr usize k_ColorComponentCount = 3;
constexpr usize k_ControlPointCompSize = 4;

/**
 * @brief Reads one typed mask chunk.
 * @tparam MaskType Specifies the mask scalar type.
 * @param maskArray Provides mask values.
 * @param offset Specifies the first tuple index.
 * @param count Specifies the number of tuple values.
 * @param maskBuffer Receives the chunk values.
 * @return Error from the mask store, or success.
 */
template <typename MaskType>
Result<> readMaskChunk(const IDataArray& maskArray, usize offset, usize count, MaskType* maskBuffer)
{
  const auto& maskStore = maskArray.getIDataStoreRefAs<AbstractDataStore<MaskType>>();
  return maskStore.copyIntoBuffer(offset, nonstd::span<MaskType>(maskBuffer, count));
}

/**
 * @struct GenerateColorArrayScanlineFunctor
 * @brief Adapts runtime scalar types to bounded RGB generation.
 */
struct GenerateColorArrayScanlineFunctor
{
  /**
   * @brief Generates RGB values for one scalar type.
   * @tparam ScalarType Specifies the scalar input type.
   * @param dataStructure Provides selected arrays.
   * @param inputValues Specifies validated color-map settings.
   * @param controlPoints Provides flattened scalar-RGB control points.
   * @param shouldCancel Stops before later chunks when true.
   * @return Error from bulk I/O, or success after cancellation.
   */
  template <typename ScalarType>
  Result<> operator()(DataStructure& dataStructure, const CreateColorMapInputValues* inputValues, const std::vector<float32>& controlPoints, const std::atomic_bool& shouldCancel)
  {
    const int numControlColors = static_cast<int>(controlPoints.size() / k_ControlPointCompSize);

    std::vector<float32> binPoints = ColorTableUtilities::NormalizeBinPoints(controlPoints);

    const auto& inputStore = dataStructure.getDataRefAs<DataArray<ScalarType>>(inputValues->SelectedDataArrayPath).getDataStoreRef();
    auto& colorStore = dataStructure.getDataRefAs<UInt8Array>(inputValues->RgbArrayPath).getDataStoreRef();
    const usize numTuples = inputStore.getNumberOfTuples();
    if(numTuples == 0)
    {
      // The Direct caller discards the empty-array result. Both paths return
      // success without writing colors.
      return {};
    }

    const IDataArray* maskArray = inputValues->UseMask ? dataStructure.getDataAs<IDataArray>(inputValues->MaskArrayPath) : nullptr;
    const bool hasBoolMask = maskArray != nullptr && maskArray->getDataType() == DataType::boolean;
    const bool hasUInt8Mask = maskArray != nullptr && maskArray->getDataType() == DataType::uint8;

    auto inputBuffer = std::make_unique<ScalarType[]>(k_ChunkTuples);
    auto colorBuffer = std::make_unique<uint8[]>(k_ChunkTuples * k_ColorComponentCount);
    auto boolMaskBuffer = hasBoolMask ? std::make_unique<bool[]>(k_ChunkTuples) : nullptr;
    auto uint8MaskBuffer = hasUInt8Mask ? std::make_unique<uint8[]>(k_ChunkTuples) : nullptr;

    ScalarType arrayMin{};
    ScalarType arrayMax{};
    bool initializedRange = false;
    // The first pass fixes the typed range without materializing the source array.
    {
      for(usize offset = 0; offset < numTuples; offset += k_ChunkTuples)
      {
        if(shouldCancel)
        {
          return {};
        }

        const usize count = std::min(k_ChunkTuples, numTuples - offset);
        Result<> readResult = inputStore.copyIntoBuffer(offset, nonstd::span<ScalarType>(inputBuffer.get(), count));
        if(readResult.invalid())
        {
          return readResult;
        }

        usize index = 0;
        if(!initializedRange)
        {
          arrayMin = inputBuffer[0];
          arrayMax = inputBuffer[0];
          initializedRange = true;
          index = 1;
        }
        for(; index < count; index++)
        {
          if(inputBuffer[index] < arrayMin)
          {
            arrayMin = inputBuffer[index];
          }
          if(inputBuffer[index] > arrayMax)
          {
            arrayMax = inputBuffer[index];
          }
        }
      }
    }

    // Unsupported mask types leave RGB output unchanged in both paths.
    if(maskArray != nullptr && !hasBoolMask && !hasUInt8Mask)
    {
      return {};
    }

    // The second pass maps values and writes complete RGB chunks.
    {
      for(usize offset = 0; offset < numTuples; offset += k_ChunkTuples)
      {
        if(shouldCancel)
        {
          return {};
        }

        const usize count = std::min(k_ChunkTuples, numTuples - offset);
        Result<> inputReadResult = inputStore.copyIntoBuffer(offset, nonstd::span<ScalarType>(inputBuffer.get(), count));
        if(inputReadResult.invalid())
        {
          return inputReadResult;
        }

        if(hasBoolMask)
        {
          Result<> maskReadResult = readMaskChunk(*maskArray, offset, count, boolMaskBuffer.get());
          if(maskReadResult.invalid())
          {
            return maskReadResult;
          }
        }
        else if(hasUInt8Mask)
        {
          Result<> maskReadResult = readMaskChunk(*maskArray, offset, count, uint8MaskBuffer.get());
          if(maskReadResult.invalid())
          {
            return maskReadResult;
          }
        }

        for(usize index = 0; index < count; index++)
        {
          const usize colorIndex = index * k_ColorComponentCount;
          const bool valid = maskArray == nullptr || (hasBoolMask ? boolMaskBuffer[index] : uint8MaskBuffer[index] != 0);
          if(!valid)
          {
            colorBuffer[colorIndex] = inputValues->InvalidColor[0];
            colorBuffer[colorIndex + 1] = inputValues->InvalidColor[1];
            colorBuffer[colorIndex + 2] = inputValues->InvalidColor[2];
            continue;
          }

          const float32 normalizedValue = ColorTableUtilities::NormalizeValue(inputBuffer[index], arrayMin, arrayMax);
          const std::array<uint8, 3> rgb = ColorTableUtilities::ComputeRgbFromControlPoints(normalizedValue, binPoints, controlPoints, static_cast<usize>(numControlColors));
          colorBuffer[colorIndex] = rgb[0];
          colorBuffer[colorIndex + 1] = rgb[1];
          colorBuffer[colorIndex + 2] = rgb[2];
        }

        Result<> writeResult = colorStore.copyFromBuffer(offset * k_ColorComponentCount, nonstd::span<const uint8>(colorBuffer.get(), count * k_ColorComponentCount));
        if(writeResult.invalid())
        {
          return writeResult;
        }
      }
    }

    return {};
  }
};
} // namespace

CreateColorMapScanline::CreateColorMapScanline(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                               const CreateColorMapInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

CreateColorMapScanline::~CreateColorMapScanline() noexcept = default;

Result<> CreateColorMapScanline::operator()()
{
  const IDataArray& selectedIDataArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SelectedDataArrayPath);

  auto controlPointsResult = ColorTableUtilities::ExtractControlPoints(m_InputValues->PresetName);
  if(controlPointsResult.invalid())
  {
    const auto& error = *controlPointsResult.errors().begin();
    return MakeErrorResult(error.code, error.message);
  }
  const auto& controlPoints = controlPointsResult.value();
  if(controlPoints.empty())
  {
    return MakeErrorResult(-34380, fmt::format("No valid points found from preset {}", m_InputValues->PresetName));
  }
  if(controlPoints.size() < 8)
  {
    return MakeErrorResult(-34382, fmt::format("Preset '{}' must define at least 2 control colors", m_InputValues->PresetName));
  }

  return ExecuteDataFunction(GenerateColorArrayScanlineFunctor{}, selectedIDataArray.getDataType(), m_DataStructure, m_InputValues, controlPoints, m_ShouldCancel);
}
