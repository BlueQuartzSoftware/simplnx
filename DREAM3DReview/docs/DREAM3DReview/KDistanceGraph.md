# KDistanceGraph

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | MinDist | K<sup>th</sup> Nearest Neighbor | int32 | Int32Parameter |
| YES | DistanceMetric | Distance Metric | ChoicesParameter::ValueType | ChoicesParameter |
| YES | UseMask | Use Mask | bool | BoolParameter |
| YES | MaskArrayPath | Mask | DataPath | ArraySelectionParameter |
| YES | SelectedArrayPath | Input Attribute Array | DataPath | ArraySelectionParameter |
| YES | KDistanceArrayPath | K Distance | DataPath | ArrayCreationParameter |
