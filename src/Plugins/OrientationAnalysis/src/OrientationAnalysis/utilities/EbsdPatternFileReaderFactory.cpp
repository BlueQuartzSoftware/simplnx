#include "OrientationAnalysis/utilities/EbsdPatternFileReaderFactory.hpp"

#include "OrientationAnalysis/utilities/EbsdPatternFileUtilities.hpp"
#include "OrientationAnalysis/utilities/EdaxUpPatternFileReader.hpp"

#include <fmt/format.h>

namespace nx::core
{
Result<std::unique_ptr<IEbsdPatternFileReader>> CreateEbsdPatternFileReader(const std::filesystem::path& filePath)
{
  const std::string extension = EbsdPatternFileUtilities::NormalizeExtension(filePath);
  if(extension == ".up1" || extension == ".up2")
  {
    return {std::make_unique<EdaxUpPatternFileReader>(filePath)};
  }

  return MakeErrorResult<std::unique_ptr<IEbsdPatternFileReader>>(
      k_UnsupportedEbsdPatternExtensionError, fmt::format("EBSD pattern file '{}' has unsupported extension '{}'. Supported extensions are: [.up1, .up2].", filePath.string(), extension));
}
} // namespace nx::core
