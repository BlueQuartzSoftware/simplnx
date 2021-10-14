# IlluminationCorrection #

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| NO | MontageSelection | Montage Selection | <<<NOT_IMPLEMENTED>>> | MontageSelectionFilterParameter |
| YES | CellAttributeMatrixName | Input Attribute Matrix Name | StringParameter::ValueType | StringParameter |
| YES | ImageDataArrayName | Input Image Array Name | StringParameter::ValueType | StringParameter |
| YES | CorrectedImageDataArrayName | Corrected Image Name | StringParameter::ValueType | StringParameter |
| YES | BackgroundDataContainerPath | Created Data Container (Corrected) | DataPath | DataGroupCreationParameter |
| YES | BackgroundCellAttributeMatrixPath | Created Attribute Matrix (Corrected) | DataPath | DataGroupCreationParameter |
| YES | BackgroundImageArrayPath | Created Image Array Name (Corrected) | DataPath | ArrayCreationParameter |
| YES | LowThreshold | Lowest allowed Image value (Image Value) | int32 | Int32Parameter |
| YES | HighThreshold | Highest allowed Image value (Image Value) | int32 | Int32Parameter |
| YES | ApplyMedianFilter | Apply median filter to background image | bool | BoolParameter |
| YES | MedianRadius | MedianRadius | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | ApplyCorrection | Apply Background Correction to Input Images | bool | BoolParameter |
| YES | ExportCorrectedImages | Export Corrected Images | bool | BoolParameter |
| YES | OutputPath | Output Path | FileSystemPathParameter::ValueType | FileSystemPathParameter |
| YES | FileExtension | File Extension | StringParameter::ValueType | StringParameter |
