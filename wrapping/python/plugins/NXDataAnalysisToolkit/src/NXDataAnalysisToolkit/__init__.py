from .Plugin import NXDataAnalysisToolkit

__all__ = ['NXDataAnalysisToolkit', 'get_plugin']

"""
This section conditionally tries to import each filter
"""

# FILTER_START: CalculateHistogramFilter
from .CalculateHistogramFilter import CalculateHistogramFilter
__all__.append('CalculateHistogramFilter')
# FILTER_END: CalculateHistogramFilter

# FILTER_START: InterpolateGridDataFilter
from .InterpolateGridDataFilter import InterpolateGridDataFilter
__all__.append('InterpolateGridDataFilter')
# FILTER_END: InterpolateGridDataFilter

# FILTER_START: CliReaderFilter
from .CliReaderFilter import CliReaderFilter
__all__.append('CliReaderFilter')
# FILTER_END: CliReaderFilter

# FILTER_START: ContourDetectionFilter
from .ContourDetectionFilter import ContourDetectionFilter
__all__.append('ContourDetectionFilter')
# FILTER_END: ContourDetectionFilter

# FILTER_START: NPSortArray
from .NPSortArray import NPSortArray
__all__.append('NPSortArray')
# FILTER_END: NPSortArray

# FILTER_START: ReadPeregrineHDF5File
from .ReadPeregrineHDF5File import ReadPeregrineHDF5File
__all__.append('ReadPeregrineHDF5File')
# FILTER_END: ReadPeregrineHDF5File

# FILTER_START: WriteAbaqusFile
from .WriteAbaqusFile import WriteAbaqusFile
__all__.append('WriteAbaqusFile')
# FILTER_END: WriteAbaqusFile

# FILTER_START: WriteAnsysFile
from .WriteAnsysFile import WriteAnsysFile
__all__.append('WriteAnsysFile')
# FILTER_END: WriteAnsysFile

# FILTER_START: WriteMedFile
from .WriteMedFile import WriteMedFile
__all__.append('WriteMedFile')
# FILTER_END: WriteMedFile

# FILTER_START: WriteGmshFile
from .WriteGmshFile import WriteGmshFile
__all__.append('WriteGmshFile')
# FILTER_END: WriteGmshFile

# FILTER_START: WriteTetGenFile
from .WriteTetGenFile import WriteTetGenFile
__all__.append('WriteTetGenFile')
# FILTER_END: WriteTetGenFile

# FILTER_START: WriteVtuFile
from .WriteVtuFile import WriteVtuFile
__all__.append('WriteVtuFile')
# FILTER_END: WriteVtuFile

# FILTER_START: ReadMeshFile
from .ReadMeshFile import ReadMeshFile
__all__.append('ReadMeshFile')
# FILTER_END: ReadMeshFile

def get_plugin():
  return NXDataAnalysisToolkit()
