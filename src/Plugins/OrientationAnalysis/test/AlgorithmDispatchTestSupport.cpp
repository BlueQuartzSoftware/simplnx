#include "AlgorithmDispatchTestSupport.hpp"

#include "simplnx/Utilities/AlgorithmDispatch.hpp"

namespace
{
using Path = nx::core::UnitTest::AlgorithmDispatchPath;

/**
 * @class DirectPathProbe
 * @brief Records selection of the direct plugin path.
 */
class DirectPathProbe
{
public:
  explicit DirectPathProbe(Path& selectedPath)
  : m_SelectedPath(selectedPath)
  {
  }

  nx::core::Result<> operator()()
  {
    m_SelectedPath = Path::Direct;
    return {};
  }

private:
  Path& m_SelectedPath;
};

/**
 * @class ScanlinePathProbe
 * @brief Records selection of the scanline plugin path.
 */
class ScanlinePathProbe
{
public:
  explicit ScanlinePathProbe(Path& selectedPath)
  : m_SelectedPath(selectedPath)
  {
  }

  nx::core::Result<> operator()()
  {
    m_SelectedPath = Path::Scanline;
    return {};
  }

private:
  Path& m_SelectedPath;
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
