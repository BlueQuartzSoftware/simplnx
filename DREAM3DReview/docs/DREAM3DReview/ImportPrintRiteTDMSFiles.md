# ImportPrintRiteTDMSFiles

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | LayerThickness | Layer Thickness | float32 | Float32Parameter |
| YES | LaserOnArrayOption | Array Used to Determine if Laser is On | ChoicesParameter::ValueType | ChoicesParameter |
| YES | LaserOnThreshold | Laser On Threshold | float32 | Float32Parameter |
| YES | DowncastRawData | Downcast Raw Data | bool | BoolParameter |
| YES | ScaleLaserPower | Scale Laser Power | bool | BoolParameter |
| YES | PowerScalingCoefficients | Power Scaling Coefficients | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | ScalePyrometerTemperature | Scale Pyrometer Tempeature | bool | BoolParameter |
| YES | TemperatureScalingCoefficients | Temperature Scaling Coefficients | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | SpatialTransformOption | Spatial Transform Options | ChoicesParameter::ValueType | ChoicesParameter |
| YES | LayerForScaling | Layer Index For Computing Real Space Transformation | int32 | Int32Parameter |
| YES | SearchRadius | Search Radius for Finding Regions | float32 | Float32Parameter |
| YES | SplitRegions1 | Split Contiguous Regions into Separate Files | bool | BoolParameter |
| YES | SplitRegions2 | Split Contiguous Regions into Separate Files | bool | BoolParameter |
| YES | STLFilePath1 | Build STL File | FileSystemPathParameter::ValueType | FileSystemPathParameter |
| YES | STLFilePath2 | Build STL File | FileSystemPathParameter::ValueType | FileSystemPathParameter |
| YES | InputSpatialTransformFilePath | Spatial Transformation Coefficients File | FileSystemPathParameter::ValueType | FileSystemPathParameter |
| YES | InputFilesList | Input PrintRite Files | GeneratedFileListParameter::ValueType | GeneratedFileListParameter |
| YES | OutputDirectory | Output File Directory | FileSystemPathParameter::ValueType | FileSystemPathParameter |
| YES | OutputFilePrefix | Output File Prefix | StringParameter::ValueType | StringParameter |
