from .Plugin import DataAnalysisToolkit

__all__ = ['NXDataAnalysisToolkit', 'get_plugin']

"""
This section conditionally tries to import each filter
"""

# FILTER_START: CalculateHistogramFilter
try:
  from .CalculateHistogramFilter import CalculateHistogramFilter
  __all__.append('CalculateHistogramFilter')
except ImportError:
  pass
# FILTER_END: CalculateHistogramFilter

# FILTER_START: InterpolateGridDataFilter
try:
  from .InterpolateGridDataFilter import InterpolateGridDataFilter
  __all__.append('InterpolateGridDataFilter')
except ImportError:
  pass
# FILTER_END: InterpolateGridDataFilter

# FILTER_START: CliReaderFilter
try:
  from .CliReaderFilter import CliReaderFilter
  __all__.append('CliReaderFilter')
except ImportError:
  pass
# FILTER_END: CliReaderFilter

# FILTER_START: ContourDetectionFilter
try:
  from .ContourDetectionFilter import ContourDetectionFilter
  __all__.append('ContourDetectionFilter')
except ImportError:
  pass
# FILTER_END: ContourDetectionFilter

# FILTER_START: NPSortArray
try:
  from .NPSortArray import NPSortArray
  __all__.append('NPSortArray')
except ImportError:
  pass
# FILTER_END: NPSortArray

# FILTER_START: ReadPeregrineHDF5File
try:
  from .ReadPeregrineHDF5File import ReadPeregrineHDF5File
  __all__.append('ReadPeregrineHDF5File')
except ImportError:
  pass
# FILTER_END: ReadPeregrineHDF5File

# FILTER_START: WriteAbaqusFile
try:
  from .WriteAbaqusFile import WriteAbaqusFile
  __all__.append('WriteAbaqusFile')
except ImportError:
  pass
# FILTER_END: WriteAbaqusFile

# FILTER_START: WriteAnsysFile
try:
  from .WriteAnsysFile import WriteAnsysFile
  __all__.append('WriteAnsysFile')
except ImportError:
  pass
# FILTER_END: WriteAnsysFile

# FILTER_START: WriteMedFile
try:
  from .WriteMedFile import WriteMedFile
  __all__.append('WriteMedFile')
except ImportError:
  pass
# FILTER_END: WriteMedFile

# FILTER_START: WriteGmshFile
try:
  from .WriteGmshFile import WriteGmshFile
  __all__.append('WriteGmshFile')
except ImportError:
  pass
# FILTER_END: WriteGmshFile

# FILTER_START: WriteTetGenFile
try:
  from .WriteTetGenFile import WriteTetGenFile
  __all__.append('WriteTetGenFile')
except ImportError:
  pass
# FILTER_END: WriteTetGenFile

# FILTER_START: WriteVtuFile
try:
  from .WriteVtuFile import WriteVtuFile
  __all__.append('WriteVtuFile')
except ImportError:
  pass
# FILTER_END: WriteVtuFile

# FILTER_START: ReadMeshFile
try:
  from .ReadMeshFile import ReadMeshFile
  __all__.append('ReadMeshFile')
except ImportError:
  pass
# FILTER_END: ReadMeshFile

def get_plugin():
  return NXDataAnalysisToolkit()
