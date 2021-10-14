# EMsoftSO3Sampler #

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | sampleModeSelector | Select the desired SO(3) sampling mode | ChoicesParameter::ValueType | ChoicesParameter |
| YES | PointGroup | Point group number (see documentation for list) | int32 | Int32Parameter |
| YES | OffsetGrid | Offset sampling grid from origin? | bool | BoolParameter |
| YES | MisOr | Misorientation angle (degree) | float64 | Float64Parameter |
| YES | RefOr | Reference orientation (Euler, °) | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | MisOrFull | Misorientation angle (degree) | float64 | Float64Parameter |
| YES | RefOrFull | Reference orientation (Euler, °) | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | Numsp | Number of sampling points along cube semi-axis | int32 | Int32Parameter |
| YES | EulerAnglesArrayName | Euler Angles | DataPath | ArrayCreationParameter |
