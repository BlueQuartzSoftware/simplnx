#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/utilities/IEbsdPatternFileReader.hpp"

#include <filesystem>

namespace nx::core
{
inline constexpr int32 k_UnsupportedUpVersionError = -78010;
inline constexpr int32 k_InvalidUpDimensionsError = -78011;
inline constexpr int32 k_InvalidUpDataOffsetError = -78012;
inline constexpr int32 k_InvalidUpPayloadError = -78013;
inline constexpr int32 k_UpDestinationMismatchError = -78014;
inline constexpr int32 k_UpPayloadReadError = -78015;
inline constexpr int32 k_UpExtensionMismatchError = -78016;
inline constexpr int32 k_UnusualUpDataOffsetWarning = -78030;
inline constexpr int32 k_UnknownUpStepWarning = -78031;
inline constexpr int32 k_ExtraUpPatternsWarning = -78032;
inline constexpr int32 k_ExtraUpPatternsSizeWarning = -78033;
inline constexpr int32 k_FutureUpVersionWarning = -78034;

class ORIENTATIONANALYSIS_EXPORT EdaxUpPatternFileReader : public IEbsdPatternFileReader
{
public:
  explicit EdaxUpPatternFileReader(std::filesystem::path filePath);
  ~EdaxUpPatternFileReader() noexcept override = default;

  Result<EbsdPatternFileInfo> readFileInfo() const override;
  Result<> readPatternData(const EbsdPatternFileInfo& fileInfo, IDataArray& outputArray, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler) const override;

private:
  std::filesystem::path m_FilePath;
};
} // namespace nx::core
