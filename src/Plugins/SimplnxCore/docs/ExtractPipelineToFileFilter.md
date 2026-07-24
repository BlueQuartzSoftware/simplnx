# Extract DREAM3D-NX Pipeline To File

## Group (Subgroup)

IO (Output)

## Description

This **Filter** reads the pipeline from an hdf5 file with the .dream3d extension and writes it back out to a json formatted pipeline file with the appropriate extension based on whether the pipeline is a DREAM3D-NX version 6 (.json) or DREAM3D-NX version 7 (.d3dpipeline) formatted pipeline.

A `.dream3d` file *may* contain an embedded copy of the pipeline that produced it, but this is not guaranteed (for example, files written by external tools, or written with pipeline embedding disabled, will not have one). If the selected `.dream3d` file does not contain an embedded pipeline, this **Filter** has nothing to extract and will report an error rather than write an empty file.

### Required Input Sources

None -- the input is a `.dream3d` file selected by file path, not a **DataStructure** object created by an upstream filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

ExtractPipelineToFile

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
