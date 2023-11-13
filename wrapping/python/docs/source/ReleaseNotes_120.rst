Release Notes 1.2.0
===================

The `complex` library is under activate development and while we strive to maintain a stable API bugs are
found the necessitate the changing of the API.

Version 1.2.0
-------------

API Additions 1.2.0
^^^^^^^^^^^^^^^^^^^

  .. code:: python
  
    cx.Pipeline().to_file(name, file_path)

Filter Changes 1.2.0
^^^^^^^^^^^^^^^^^^^^

- All import and export filters have been renamed to either *ReadXXXX* or *WriteXXXX*. This will effect the python filter classes. 

- ComplexCore

+----------------------------------------+---------------------------------------+-----------+
| Old File Name                          | New File Name                         | Type      |
+========================================+=======================================+===========+
| AbaqusHexahedronWriterFilter           | WriteAbaqusHexahedronFilter           | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| AvizoRectilinearCoordinateWriterFilter | WriteAvizoRectilinearCoordinateFilter | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| AvizoUniformCoordinateWriterFilter     | WriteAvizoUniformCoordinateFilter     | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| ExportDREAM3DFilter                    | WriteDREAM3DFilter                    | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| FeatureDataCSVWriterFilter             | WriteFeatureDataCSVFilter             | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| ImportDeformKeyFileV12Filter           | ReadDeformKeyFileV12Filter            | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| ImportVolumeGraphicsFileFilter         | ReadVolumeGraphicsFileFilter          | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| ImportBinaryCTNorthstarFilter          | ReadBinaryCTNorthstarFilter           | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| LosAlamosFFTWriterFilter               | WriteLosAlamosFFTFilter               | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| RawBinaryReaderFilter                  | ReadRawBinaryFilter                   | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| StlFileReaderFilter                    | ReadStlFileFilter                     | Filter    |
+----------------------------------------+---------------------------------------+-----------+
| VtkRectilinearGridWriterFilter         | WriteVtkRectilinearGridFilter         | Filter    |
+----------------------------------------+---------------------------------------+-----------+

- OrientationAnalysis

+------------------------------+-----------------------------+-----------+
| Old File Name                | New File Name               | Type      |
+==============================+=============================+===========+
| EnsembleInfoReaderFilter     | ReadEnsembleInfoFilter      | Filter    |
+------------------------------+-----------------------------+-----------+
| ExportGBCDGMTFileFilter      | WriteGBCDGMTFileFilter      | Filter    |
+------------------------------+-----------------------------+-----------+
| ExportGBCDTriangleDataFilter | WriteGBCDTriangleDataFilter | Filter    |
+------------------------------+-----------------------------+-----------+
| ImportH5Data                 | ReadH5Data                  | Utility   |
+------------------------------+-----------------------------+-----------+
| ImportH5OimDataFilter        | ReadH5OimDataFilter         | Filter    |
+------------------------------+-----------------------------+-----------+
| INLWriterFilter              | WriteINLFileFilter          | Filter    |
+------------------------------+-----------------------------+-----------+

