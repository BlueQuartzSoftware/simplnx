#include "AlgorithmDispatchTestSupport.hpp"

#include "simplnx/Utilities/AlgorithmDispatch.hpp"

namespace
{
using PathType = nx::core::UnitTest::AlgorithmDispatchPath;

/**
 * @class DirectPathProbe
 * @brief Records selection of the direct plugin path.
 */
class DirectPathProbe
{
public:
  explicit DirectPathProbe(PathType& selectedPath)
  : m_SelectedPath(selectedPath)
  {
  }

  nx::core::Result<> operator()()
  {
    m_SelectedPath = PathType::Direct;
    return {};
  }

private:
  PathType& m_SelectedPath;
};

/**
 * @class ScanlinePathProbe
 * @brief Records selection of the scanline plugin path.
 */
class ScanlinePathProbe
{
public:
  explicit ScanlinePathProbe(PathType& selectedPath)
  : m_SelectedPath(selectedPath)
  {
  }

  nx::core::Result<> operator()()
  {
    m_SelectedPath = PathType::Scanline;
    return {};
  }

private:
  PathType& m_SelectedPath;
};
} // namespace

namespace nx::core::UnitTest
{
AlgorithmDispatchPath GetAlgorithmDispatchPathFromOrientationAnalysisPlugin()
{
  AlgorithmDispatchPath selectedPath = AlgorithmDispatchPath::Unknown;
  const Result<> result = DispatchAlgorithm<DirectPathProbe, ScanlinePathProbe>({}, selectedPath);
  if(result.invalid())
  {
    return AlgorithmDispatchPath::Unknown;
  }
  return selectedPath;
}
} // namespace nx::core::UnitTest
