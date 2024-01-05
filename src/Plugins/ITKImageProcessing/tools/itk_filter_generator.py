import json
from os import remove
import sys
import argparse
import textwrap
import re
import uuid

from string import Template
from typing import Dict, List, Optional
from dataclasses import dataclass, field
from pathlib import Path
from abc import ABC, abstractmethod

PLUGIN_NAME: str = 'ITKImageProcessing'
PLUGIN_NAME_UPPER: str = PLUGIN_NAME.upper()

FILTERS: Dict[str, str] = {
    'AbsImage': '09f45c29-1cfb-566c-b3ae-d832b4f95905',
    'AcosImage': 'b09ec654-87a5-5dfa-9949-aa69f1fbfdd1',
    'AdaptiveHistogramEqualizationImage': '2d5a7599-5e01-5489-a107-23b704d2b5eb',
    'ApproximateSignedDistanceMapImage': '066712e3-0378-566e-8236-1796c88d5e02',
    'AsinImage': '79509ab1-24e1-50e4-9351-c5ce7cd87a72',
    'AtanImage': 'e938569d-3644-5d00-a4e0-ab327937457d',
    'BilateralImage': '18ab754c-3219-59c8-928e-5fb4a09174e0',
    'BinaryClosingByReconstructionImage': '0cf0698d-65eb-58bb-9f89-51e875432197',
    'BinaryContourImage': '3c451ac9-bfef-5e41-bae9-3957a0fc26a1',
    'BinaryDilateImage': 'f86167ad-a1a1-557b-97ea-92a3618baa8f',
    'BinaryErodeImage': '522c5249-c048-579a-98dd-f7aadafc5578',
    'BinaryMorphologicalClosingImage': '1d8deea7-c6d0-5fa1-95cb-b14f19df97e8',
    'BinaryMorphologicalOpeningImage': '704c801a-7549-54c4-9def-c4bb58d07fd1',
    'BinaryOpeningByReconstructionImage': 'bd1c2353-0a39-52c0-902b-ee64721994c7',
    'BinaryProjectionImage': '606c3700-f793-5852-9a0f-3123bd212447',
    'BinaryThinningImage': 'dcceeb50-5924-5eae-88ea-34793cf545a9',
    'BinaryThresholdImage': 'ba8a3f2e-3963-57c0-a8da-239e25de0526',
    'BinomialBlurImage': '4f51765f-ee36-52d0-96b6-f2aca3c24e7c',
    'BlackTopHatImage': 'e26e7359-f72c-5924-b42e-dd5dd454a794',
    'BoundedReciprocalImage': '17f60a64-de93-59aa-a5e2-063e0aadafb7',
    'BoxMeanImage': '6138f949-e363-5ca3-bc31-4f3daed65da5',
    'ClosingByReconstructionImage': '99a7aa3c-f945-5e77-875a-23b5231ab3f4',
    'ConnectedComponentImage': 'bf554dd5-a927-5969-b651-1c47d386afce',
    'CosImage': '2c2d7bf6-1e78-52e6-80aa-58b504ce0912',
    'CurvatureAnisotropicDiffusionImage': '009fb2d0-6f65-5406-bb2a-4a883d0bc18c',
    'CurvatureFlowImage': '653f26dd-a5ef-5c75-b6f6-bc096268f25e',
    'DanielssonDistanceMapImage': '53d5b289-a716-559b-89d9-5ebb34f714ca',
    'DilateObjectMorphologyImage': 'dbf29c6d-461c-55e7-a6c4-56477d9da55b',
    'DiscreteGaussianImage': '53df5340-f632-598f-8a9b-802296b3a95c',
    'DoubleThresholdImage': '7fcfa231-519e-58da-bf8f-ad0f728bf0fe',
    'ErodeObjectMorphologyImage': 'caea0698-4253-518b-ab3f-8ebc140d92ea',
    'ExpImage': 'a6fb3f3a-6c7a-5dfc-a4f1-75ff1d62c32f',
    'ExpNegativeImage': '634c2306-c1ee-5a45-a55c-f8286e36999a',
    'FFTNormalizedCorrelationImage': 'a0d962b7-9d5c-5abc-a078-1fe795df4663',
    'GradientAnisotropicDiffusionImage': '98d0b50b-9639-53a0-9e30-2fae6f7ab869',
    'GradientMagnitudeImage': '3aa99151-e722-51a0-90ba-71e93347ab09',
    'GradientMagnitudeRecursiveGaussianImage': 'fd688b32-d90e-5945-905b-2b7187b46265',
    'GrayscaleDilateImage': '66cec151-2950-51f8-8a02-47d3516d8721',
    'GrayscaleErodeImage': 'aef4e804-3f7a-5dc0-911c-b1f16a393a69',
    'GrayscaleFillholeImage': '54c8dd45-88c4-5d4b-8a39-e3cc595e1cf8',
    'GrayscaleGrindPeakImage': 'd910551f-4eec-55c9-b0ce-69c2277e61bd',
    'GrayscaleMorphologicalClosingImage': '849a1903-5595-5029-bbde-6f4b68b2a25c',
    'GrayscaleMorphologicalOpeningImage': 'c88ac42b-9477-5088-9ec0-862af1e0bb56',
    'HConvexImage': '8bc34707-04c0-5e83-8583-48ee19306a1d',
    'HMaximaImage': '932a6df4-212e-53a1-a2ab-c29bd376bb7b',
    'HMinimaImage': 'f1d7cf59-9b7c-53cb-b71a-76cf91c86e8f',
    'HistogramMatchingImage': '33ca886c-42b9-524a-984a-dab24f8bb74c',
    'IntensityWindowingImage': '4faf4c59-6f29-53af-bc78-5aecffce0e37',
    'InvertIntensityImage': 'c6e10fa5-5462-546b-b34b-0f0ea75a7e43',
    'IsoContourDistanceImage': 'e9952e0e-97e4-53aa-8c6c-115fd18f0b89',
    'LabelContourImage': '668f0b90-b504-5fba-b648-7c9677e1f452',
    'LaplacianRecursiveGaussianImage': '9677659d-b08c-58a4-ac4d-fba7d9093454',
    'LaplacianSharpeningImage': 'c4963181-c788-5efc-8560-d005a5e01eea',
    'Log10Image': 'dbfd1a57-2a17-572d-93a7-8fd2f8e92eb0',
    'LogImage': '69aba77c-9a35-5251-a18a-e3728ddd2963',
    'MaskImage': '97102d65-9c32-576a-9177-c59d958bad10',
    'MaximumProjectionImage': 'b2cb7ad7-6f62-51c4-943d-54d19c64e7be',
    'MeanProjectionImage': '85c061bc-3ad7-5abc-8fc7-140678311706',
    'MedianImage': 'cc27ee9a-9946-56ad-afd4-6e98b71f417d',
    'MedianProjectionImage': '1a7e8483-5838-585c-8d71-e79f836133c4',
    'MinMaxCurvatureFlowImage': 'bd9bdf46-a229-544a-b158-151920261a63',
    'MinimumProjectionImage': '6394a737-4a31-5593-9bb5-28492129ccf7',
    'MorphologicalGradientImage': '12c83608-c4c5-5c72-b22f-a7696e3f5448',
    'MorphologicalWatershedFromMarkersImage': '81308863-094b-511d-9aa9-865e02e2b8d5',
    'MorphologicalWatershedImage': 'b2248340-a371-5899-90a2-86047950f0a2',
    'NormalizeImage': '5b905619-c46b-5690-b6fa-8e97cf4537b8',
    'NormalizeToConstantImage': '001dd629-7032-56a9-99ec-ffaf2caf2ab0',
    'NotImage': 'c8362fb9-d3ab-55c0-902b-274cc27d9bb8',
    'OpeningByReconstructionImage': 'ca04004f-fb11-588d-9f77-d00b3ee9ad2a',
    'OtsuMultipleThresholdsImage': '6e66563a-edcf-5e11-bc1d-ceed36d8493f',
    'PatchBasedDenoisingImage': 'ed61aebd-3a47-5ee1-8c9e-4ce205111b76',
    'RegionalMaximaImage': '9af89118-2d15-54ca-9590-75df8be33317',
    'RegionalMinimaImage': 'f8ed09ae-1f84-5668-a4ad-d770e264f01e',
    'RelabelComponentImage': '4398d76d-c9aa-5161-bb48-92dd9daaa352',
    'RescaleIntensityImage': '77bf2192-851d-5127-9add-634c1ef4f67f',
    'SaltAndPepperNoiseImage': '602c270d-ec55-580c-9108-785ba8530964',
    'ShiftScaleImage': '31d325fa-e605-5415-85ab-ab93e8cbf32f',
    'ShotNoiseImage': '97f20f54-276b-54f3-87c9-5eaf16e6c4df',
    'SigmoidImage': 'e6675be7-e98d-5e0f-a088-ba15cc301038',
    'SignedDanielssonDistanceMapImage': 'fc192fa8-b6b7-539c-9763-f683724da626',
    'SignedMaurerDistanceMapImage': 'bb15d42a-3077-582a-be1a-76b2bae172e9',
    'SinImage': '1eb4b4f7-1704-58e4-9f78-8726a5c8c302',
    'SmoothingRecursiveGaussianImage': '0fd06492-06b1-5044-964c-e0555c556327',
    'SpeckleNoiseImage': '764085a4-6ecb-5fb7-891d-2fda208ba5d8',
    'SqrtImage': '8087dcad-68f2-598b-9670-d0f57647a445',
    'SquareImage': 'f092420e-14a0-5dc0-91f8-de0082103aef',
    'StandardDeviationProjectionImage': '89b327a7-c6a0-5965-b8aa-9d8bfcedcc76',
    'SumProjectionImage': '40714670-b3bd-554c-badb-787d8bab7568',
    'TanImage': '672810d9-5ec0-59c1-a209-8fb56c7a018a',
    'ThresholdImage': '5845ee06-5c8a-5a74-80fb-c820bd8dfb75',
    'ThresholdMaximumConnectedComponentsImage': '2a531add-79fd-5c07-98a6-f875d2ecef4e',
    'ValuedRegionalMaximaImage': '10aff542-81c5-5f09-9797-c7171c40b6a0',
    'ValuedRegionalMinimaImage': '739a0908-cb60-50f7-a484-b2157d023093',
    'VectorConnectedComponentImage': '9d42ede4-fd4b-57fe-9595-50c16deaa3a2',
    'WhiteTopHatImage': '02e059f7-8055-52b4-9d48-915b67d1e39a',
    'ZeroCrossingImage': '0259fa1a-4706-5df1-8418-95ffc7b932dd',
}

def fixup_detail_desc(detail_desc: str, max_line_length: int = 200) -> str:
    detail_desc = detail_desc.replace('\\see', '@see')
    detail_desc = detail_desc.replace('\\author', '@author')
    detail_desc = detail_desc.replace('\\li', '@li')
    detail_desc = detail_desc.replace('\n', '\n * ')
    lines = detail_desc.split('\n')
    fixed_lines: List[str] = []
    for line in lines:
        line = line.rstrip()
        if len(line) > max_line_length:
            wrapped_lines = textwrap.wrap(line, width=max_line_length, initial_indent='', subsequent_indent=' * ')
            fixed_lines.extend(wrapped_lines)
        else:
            fixed_lines.append(line)
    detail_desc_str = '\n'.join(fixed_lines)
    return detail_desc_str

@dataclass
class PropertyData:
    name: str
    type: str
    itk_type: str
    default: str
    briefdescriptionSet: str
    detaileddescriptionSet: str

@dataclass
class InputData:
    name: str
    type: str
    default: str
    is_image: bool

TYPE_STR_DICT: Dict[str, str] = {
    'uint8_t': 'uint8',
    'uint16_t': 'uint16',
    'uint32_t': 'uint32',
    'uint64_t': 'uint64',
    'size_t': 'usize',
    'int8_t': 'int8',
    'int16_t': 'int16',
    'int32_t': 'int32',
    'int64_t': 'int64',
    'float': 'float32',
    'double': 'float64',
    'unsigned int': 'uint32',
    'int': 'int32',
}

def fixup_type(type_str: str) -> str:
    return TYPE_STR_DICT[type_str] if type_str in TYPE_STR_DICT else type_str

@dataclass
class SettingsData:
    parameter: str
    value: str

@dataclass
class TestData:
    tag: str
    description: str
    settings: List[SettingsData]
    md5hash: str
    inputs: List[str]
    tolerance: str

@dataclass
class FilterData:
    name: str
    brief_desc: str
    detail_desc: str
    itk_module: str
    itk_group: str
    members: List[PropertyData]
    inputs: List[InputData]
    is_projection: bool
    pixel_types: str
    tests: List[TestData]
    filter_type: str
    output_pixel_type: str
    vector_pixel_types_by_component: str

    base_name: str = field(init=False)
    filter_name: str = field(init=False)
    human_name: str = field(init=False)
    default_tags: List[str] = field(init=False)

    def __post_init__(self):
        self.base_name = self.name.replace('Filter', '')
        self.filter_name = f'ITK{self.base_name}'

        a = re.compile('((?<=[a-z0-9])[A-Z]|(?!^)[A-Z](?=[a-z]))')
        self.human_name = a.sub(r' \1', self.name)
        self.human_name = f'ITK {self.human_name}'
        # self.human_name = self.human_name.replace('Filter', ' Filter')
        # self.human_name = self.human_name.replace('Image', ' Image')
        self.default_tags = [PLUGIN_NAME, self.filter_name, self.itk_module, self.itk_group]

def read_filter_json(json_dir: Path, filter_name: str) -> FilterData:
    with open(f'{json_dir}/{filter_name}Filter.json', 'r') as json_file:
        filter_json = json.load(json_file)
        name: str = filter_json['name']
        pixel_types = filter_json['pixel_types']
        brief_desc: str = filter_json['briefdescription']
        detail_desc: str = filter_json['detaileddescription']
        itk_module: str = filter_json['itk_module']
        itk_group: str = filter_json['itk_group']
        filter_type: str = filter_json['filter_type'] if 'filter_type' in filter_json else ''
        output_pixel_type: str = filter_json['output_pixel_type'] if 'output_pixel_type' in filter_json else ''
        vector_pixel_types_by_component: str = filter_json['vector_pixel_types_by_component'] if 'vector_pixel_types_by_component' in filter_json else ''
        members: List[PropertyData] = []
        for item in filter_json['members']:
            prop_name: str = item['name']
            prop_type: str = fixup_type(item['type']) if 'type' in item else ''
            prop_itk_type: str = fixup_type(item['itk_type']) if 'itk_type' in item else ''
            prop_default: str = item['default']
            briefdescription_set: str = item['briefdescriptionSet'] if 'briefdescriptionSet' in item else ''
            detaileddescription_set: str = item['detaileddescriptionSet'] if 'detaileddescriptionSet' in item else ''
            prop = PropertyData(
                name=prop_name,
                type=prop_type,
                itk_type=prop_itk_type,
                default=prop_default,
                briefdescriptionSet=briefdescription_set,
                detaileddescriptionSet=detaileddescription_set
            )
            members.append(prop)

        inputs: List[InputData] = []
        if 'inputs' in filter_json:
            for item in filter_json['inputs']:
                input_name: str = item['name']
                input_type: str = fixup_type(item['type'])
                input_default: str = item['default'] if 'default' in item else ''
                input_is_image: bool = input_name == 'Image'
                input_data = InputData(
                    name=input_name,
                    type=input_type,
                    default=input_default,
                    is_image=input_is_image,
                )
                inputs.append(input_data)

        tests: List[TestData] = []
        for item in filter_json['tests']:
            settings: List[SettingsData] = []
            if 'settings' in item:
                for setting in item['settings']:
                    param: str = setting['parameter']
                    value: str = setting['value']
                    setting = SettingsData(
                        parameter=param,
                        value=value,
                    )
                    settings.append(setting)

            tag: str = item['tag']
            description: str = item['description']
            md5hash: str = item['md5hash'] if 'md5hash' in item else ''

            test_inputs: List[str] = [input for input in item['inputs']]

            tolerance: str = item['tolerance'] if 'tolerance' in item else ''

            test = TestData(
                tag=tag,
                description=description,
                settings=settings,
                md5hash=md5hash,
                inputs=test_inputs,
                tolerance=tolerance,
            )
            tests.append(test)

        is_projection = 'Projection' in filter_json

    return FilterData(
        name=name,
        brief_desc=brief_desc,
        detail_desc=detail_desc,
        itk_module=itk_module,
        itk_group=itk_group,
        members=members,
        inputs=inputs,
        is_projection=is_projection,
        pixel_types=pixel_types,
        tests=tests,
        filter_type=filter_type,
        output_pixel_type=output_pixel_type,
        vector_pixel_types_by_component=vector_pixel_types_by_component,
    )

def camel_to_snake(name):
    name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()

def generate_key_name(name: str, is_image: bool) -> str:
    suffix = 'DataPath' if is_image else ''
    key_name = f'{name}{suffix}'
    return key_name

def generate_key_var(name: str, is_image: bool) -> str:
    key_name = generate_key_name(name, is_image)
    return f'k_{key_name}_Key'

def generate_string_literal(name: str, indent: int = 2, is_image: bool = False) -> str:
    key_name = generate_key_name(name, is_image)
    key_name = camel_to_snake(key_name)

    key_var = generate_key_var(name, is_image)
    return f'{str():<{indent}}static inline constexpr StringLiteral {key_var} = \"{key_name}\";'

def generate_properties(filter_data: FilterData) -> List[str]:
    return [generate_string_literal(member.name) for member in filter_data.members]

def generate_inputs(filter_data: FilterData) -> List[str]:
    return [generate_string_literal(input_data.name, is_image=input_data.is_image) for input_data in filter_data.inputs]

def write_filter_header(filter_data: FilterData, template: Template, output_dir: Path) -> None:
    fixed_detail_desc = fixup_detail_desc(filter_data.detail_desc)

    properties = generate_properties(filter_data)
    inputs = generate_inputs(filter_data)

    parameter_keys = []
    parameter_keys.extend(properties)
    parameter_keys.extend(inputs)

    parameter_keys_str = '\n'.join(parameter_keys)

    if parameter_keys:
        parameter_keys_str = f'\n{parameter_keys_str}\n'
    else:
        parameter_keys_str = '\n'

    filter_uuid = uuid.uuid4()
    substitutions = {
        'PLUGIN_NAME': PLUGIN_NAME,
        'PLUGIN_NAME_UPPER': PLUGIN_NAME_UPPER,
        'FILTER_NAME': filter_data.filter_name,
        'BRIEF_DESCRIPTION': filter_data.brief_desc,
        'DETAILED_DESCRIPTION': fixed_detail_desc,
        'ITK_MODULE': filter_data.itk_module,
        'ITK_GROUP': filter_data.itk_group,
        'UUID': filter_uuid,
        'PARAMETER_KEYS': parameter_keys_str,
    }
    print(f'    {{simplnx::Uuid::FromString(\"{FILTERS[filter_data.base_name]}\").value(), nx::core::FilterTraits<{filter_data.filter_name}>::uuid}}, // {filter_data.filter_name}')

    output = template.substitute(substitutions)

    with open(f'{output_dir}/{filter_data.filter_name}.hpp', 'w') as output_file:
        output_file.write(output)

def make_include_str(param_name: str) -> str:
    return f'#include \"simplnx/Parameters/{param_name}.hpp\"'

def make_parameter_def_str(type_name: str, key_var: str, human_name: str, help_text: str, default: str, extra: str = '') -> str:
    extra_str = f', {extra}' if extra else ''
    help_text = help_text.replace('\n', ' ')
    help_text = help_text.replace('\"', '\'')
    return f'  params.insert(std::make_unique<{type_name}>({key_var}, \"{human_name}\", \"{help_text}\", {default}{extra_str}));'

def make_parameter_def_str_from_prop(prop: PropertyData, param_name: str) -> str:
    key_var = generate_key_var(prop.name, False)
    return make_parameter_def_str(param_name, key_var, prop.name, prop.detaileddescriptionSet, prop.default)

def make_first_letter_lowercase(text: str) -> str:
    return text[0].lower() + text[1:]

def make_preflight_def(var_name: str, var_type: str, var_key: str):
    return f'auto {var_name} = filterArgs.value<{var_type}>({var_key});'

class Parameter(ABC):
    @abstractmethod
    def get_type(self) -> str:
        raise NotImplementedError

    @abstractmethod
    def get_include_str(self) -> str:
        raise NotImplementedError

    @abstractmethod
    def get_parameter_def_str(self, prop: PropertyData) -> str:
        raise NotImplementedError

    @abstractmethod
    def get_preflight_def(self, prop: PropertyData) -> str:
        return make_preflight_def(make_first_letter_lowercase(prop.name), prop.type, generate_key_var(prop.name, False))

    @abstractmethod
    def get_test_value_str(self, setting: SettingsData) -> str:
        return f'{setting.value}'

    @abstractmethod
    def get_itk_functor_member_str(self, prop: PropertyData) -> str:
        return f'{prop.type} {make_first_letter_lowercase(prop.name)} = {prop.default};'

    @abstractmethod
    def get_itk_functor_body_str(self, prop: PropertyData) -> str:
        return f'filter->Set{prop.name}({make_first_letter_lowercase(prop.name)});'

class BasicParameter(Parameter):
    def __init__(self, parameter_include: str, parameter_name: str) -> None:
        self.parameter_name = parameter_name
        self.parameter_include = parameter_include
        super().__init__()

    def get_type(self) -> str:
        return f'{self.parameter_name}::ValueType'

    def get_include_str(self) -> str:
        return make_include_str(self.parameter_include)

    def get_parameter_def_str(self, prop: PropertyData) -> str:
        return make_parameter_def_str_from_prop(prop, self.parameter_name)

    def get_preflight_def(self, prop: PropertyData) -> str:
        return super().get_preflight_def(prop)

    def get_test_value_str(self, setting: SettingsData) -> str:
        return super().get_test_value_str(setting)

    def get_itk_functor_member_str(self, prop: PropertyData) -> str:
        return super().get_itk_functor_member_str(prop)

    def get_itk_functor_body_str(self, prop: PropertyData) -> str:
        return super().get_itk_functor_body_str(prop)

class NumberParameter(BasicParameter):
    def __init__(self, parameter_name: str) -> None:
        super().__init__('NumberParameter', parameter_name)

class ITKParameter(BasicParameter):
    def __init__(self, parameter_include: str, parameter_name: str) -> None:
        super().__init__(parameter_include, parameter_name)

class KernelRadiusParameter(Parameter):
    def get_type(self) -> str:
        return f'VectorParameter<uint32>::ValueType'

    def get_include_str(self) -> str:
        return make_include_str('VectorParameter')

    def get_parameter_def_str(self, prop: PropertyData) -> str:
        default_str = prop.default
        default_str = default_str.replace(f'std::vector<uint32_t>', 'std::vector<uint32>')
        return make_parameter_def_str('VectorParameter<uint32>', generate_key_var(prop.name, False), 'KernelRadius', 'The radius of the kernel structuring element.', default_str, extra='std::vector<std::string>{"X","Y","Z"}')

    def get_preflight_def(self, prop: PropertyData) -> str:
        return make_preflight_def(make_first_letter_lowercase(prop.name), self.get_type(), generate_key_var(prop.name, False))

    def get_test_value_str(self, setting: SettingsData) -> str:
        if isinstance(setting.value, int):
            setting.value = [setting.value, 1]
        if isinstance(setting.value, list) and len(setting.value) == 2:
            setting.value.append(1)

        values = f'{setting.value}'
        values = values.replace('[', '')
        values = values.replace(']', '')
        return f'std::vector<uint32>{{{values}}}'

    def get_itk_functor_member_str(self, prop: PropertyData) -> str:
        return f'std::vector<uint32> kernelRadius = {{1, 1, 1}};'

    def get_itk_functor_body_str(self, prop: PropertyData) -> str:
        return 'auto kernel = itk::simple::CreateKernel<Dimension>(kernelType, kernelRadius);\n    filter->SetKernel(kernel);'

class KernelTypeParameter(Parameter):
    def get_type(self) -> str:
        return f'ChoicesParameter::ValueType'

    def get_include_str(self) -> str:
        return make_include_str('ChoicesParameter')

    def get_parameter_def_str(self, prop: PropertyData) -> str:
        return make_parameter_def_str('ChoicesParameter', generate_key_var(prop.name, False), 'KernelType', '', f'static_cast<uint64>({prop.default})', 'ChoicesParameter::Choices{\"Annulus\", \"Ball\", \"Box\", \"Cross\"}')

    def get_preflight_def(self, prop: PropertyData) -> str:
        return f'auto {make_first_letter_lowercase(prop.name)} = static_cast<itk::simple::KernelEnum>(filterArgs.value<uint64>({generate_key_var(prop.name, False)}));'

    def get_test_value_str(self, setting: SettingsData) -> str:
        return super().get_test_value_str(setting)

    def get_itk_functor_member_str(self, prop: PropertyData) -> str:
        return f'itk::simple::KernelEnum kernelType = {prop.default};'

    def get_itk_functor_body_str(self, prop: PropertyData) -> str:
        return ''

class KernelEnumParameter(Parameter):
    def get_type(self) -> str:
        return ''

    def get_include_str(self) -> str:
        return ''

    def get_parameter_def_str(self, prop: PropertyData) -> str:
        return ''

    def get_preflight_def(self, prop: PropertyData) -> str:
        return super().get_preflight_def(prop)

    def get_test_value_str(self, setting: SettingsData) -> str:
        return super().get_test_value_str(setting)

    def get_itk_functor_member_str(self, prop: PropertyData) -> str:
        return super().get_itk_functor_member_str(prop)

    def get_itk_functor_body_str(self, prop: PropertyData) -> str:
        return super().get_itk_functor_body_str(prop)

class PixelIDValueEnumParameter(Parameter):
    def get_type(self) -> str:
        return ''

    def get_include_str(self) -> str:
        return make_include_str('NumberParameter')

    def get_parameter_def_str(self, prop: PropertyData) -> str:
        return ''

    def get_test_value_str(self, setting: SettingsData) -> str:
        return super().get_test_value_str(setting)

    def get_preflight_def(self, prop: PropertyData) -> str:
        return super().get_preflight_def(prop)

    def get_itk_functor_member_str(self, prop: PropertyData) -> str:
        return super().get_itk_functor_member_str(prop)

    def get_itk_functor_body_str(self, prop: PropertyData) -> str:
        return super().get_itk_functor_body_str(prop)

class NoiseModelTypeParameter(Parameter):
    def get_type(self) -> str:
        return ''

    def get_include_str(self) -> str:
        return ''

    def get_parameter_def_str(self, prop: PropertyData) -> str:
        return ''

    def get_preflight_def(self, prop: PropertyData) -> str:
        return super().get_preflight_def(prop)

    def get_test_value_str(self, setting: SettingsData) -> str:
        return super().get_test_value_str(setting)

    def get_itk_functor_member_str(self, prop: PropertyData) -> str:
        return super().get_itk_functor_member_str(prop)

    def get_itk_functor_body_str(self, prop: PropertyData) -> str:
        return super().get_itk_functor_body_str(prop)

class RadiusTypeParameter(Parameter):
    def get_type(self) -> str:
        return 'VectorUInt32Parameter::ValueType'

    def get_include_str(self) -> str:
        return '#include "simplnx/Parameters/VectorParameter.hpp"'

    def get_parameter_def_str(self, prop: PropertyData) -> str:
        return f'  params.insert(std::make_unique<VectorUInt32Parameter>(k_Radius_Key, "Radius", "Radius Dimensions XYZ", {prop.default}, std::vector<std::string>{{"X", "Y", "Z"}}));\n'

    def get_preflight_def(self, prop: PropertyData) -> str:
        return f'auto {make_first_letter_lowercase(prop.name)} = filterArgs.value<VectorUInt32Parameter::ValueType>(k_Radius_Key);\n'

    def get_test_value_str(self, setting: SettingsData) -> str:
        code = 'VectorUInt32Parameter::ValueType{'
        for item in setting.value:
            code += f'{item},'
        if len(setting.value) == 2:
            code += '0'
        return code + '}'

    def get_itk_functor_member_str(self, prop: PropertyData) -> str:
        code = f'using {prop.name}InputRadiusType = std::vector<{prop.type}>;\n'
        return code + f'  {prop.name}InputRadiusType {make_first_letter_lowercase(prop.name)} = {prop.default};'

    def get_itk_functor_body_str(self, prop: PropertyData) -> str:
        
        code = f'// Set the {prop.name} Filter Property\n'
        code += f'    {{\n'
        code +=  '      using RadiusType = typename FilterType::RadiusType;\n'
        code +=  '      auto convertedRadius = ITK::CastVec3ToITK<RadiusType, typename RadiusType::SizeValueType>(radius, RadiusType::Dimension);\n'
        code +=  '      filter->SetRadius(convertedRadius);\n'
        code +=  '    }\n'
        return code


class ArrayTypeParameter(Parameter):
    def get_type(self) -> str:
        return 'VectorFloat64Parameter::ValueType'

    def get_include_str(self) -> str:
        return '#include "simplnx/Parameters/VectorParameter.hpp"'

    def get_parameter_def_str(self, prop: PropertyData) -> str:
        return f'  params.insert(std::make_unique<VectorFloat64Parameter>(k_{prop.name}_Key, "{prop.name}", "{prop.detaileddescriptionSet}", {prop.default}, std::vector<std::string>{{"X", "Y", "Z"}}));\n'

    def get_preflight_def(self, prop: PropertyData) -> str:
        return f'auto {make_first_letter_lowercase(prop.name)} = filterArgs.value<VectorFloat64Parameter::ValueType>(k_{prop.name}_Key);\n'

    def get_test_value_str(self, setting: SettingsData) -> str:
        code = 'VectorFloat64Parameter::ValueType{'
        for item in setting.value:
            code += f'{item},'
        if len(setting.value) == 2:
            code += '0'
        return code + '}'

    def get_itk_functor_member_str(self, prop: PropertyData) -> str:
        code = f'using {prop.name}InputArrayType = std::vector<{prop.type}>;\n'
        return code + f'  {prop.name}InputArrayType {make_first_letter_lowercase(prop.name)} = {prop.default};'

    def get_itk_functor_body_str(self, prop: PropertyData) -> str:
        code = f'filter->Set{prop.name}({make_first_letter_lowercase(prop.name)}.data()); // Set the {prop.name} Filter Property'
        return code


PARAM_TYPES: Dict[str, Parameter] = {
    'KernelEnum': KernelEnumParameter(),
    'PixelIDValueEnum': KernelRadiusParameter(),
    'typename FilterType::NoiseModelType': NoiseModelTypeParameter(),
    'typename FilterType::RadiusType': RadiusTypeParameter(),
    'typename FilterType::ArrayType': ArrayTypeParameter(),
    'typename FilterType::SigmaArrayType': RadiusTypeParameter(),
    'bool': BasicParameter('BoolParameter', 'BoolParameter'),
    'uint8': NumberParameter('UInt8Parameter'),
    'uint32': NumberParameter('UInt32Parameter'),
    'uint64': NumberParameter('UInt64Parameter'),
    'int32': NumberParameter('Int32Parameter'),
    'float32': NumberParameter('Float32Parameter'),
    'float64': NumberParameter('Float64Parameter'),
}

def remove_duplicates(items: list) -> list:
    return list(dict.fromkeys(items))

def get_parameter(prop: PropertyData) -> Parameter:
    if prop.name == 'KernelRadius':
        return KernelRadiusParameter()
    elif prop.name == 'KernelType':
        return KernelTypeParameter()

    if prop.itk_type:
        return PARAM_TYPES[prop.itk_type]
    
    return PARAM_TYPES[prop.type]

def get_parameter_from_name(filter_data: FilterData, name: str) -> Parameter:
    return get_parameter(next(filter(lambda item: item.name == name, filter_data.members)))

def get_default_parameter_includes() -> List[str]:
    return ['#include \"simplnx/Parameters/DataObjectNameParameter.hpp\"',
        '#include \"simplnx/Parameters/ArraySelectionParameter.hpp\"',
        '#include \"simplnx/Parameters/GeometrySelectionParameter.hpp\"'
    ]

def get_parameter_includes(filter_data: FilterData) -> List[str]:
    includes: List[str] = []
    for item in filter_data.members:
        param = get_parameter(item)
        includes.append(param.get_include_str())
    includes = remove_duplicates(includes)
    includes = sorted(includes)
    return includes

def get_parameter_defs(filter_data: FilterData) -> List[str]:
    defs: List[str] = []
    for item in filter_data.members:
        param = get_parameter(item)
        defs.append(param.get_parameter_def_str(item))
    return defs

def get_itk_functor_name(filter_data: FilterData) -> str:
    return f'{filter_data.filter_name}Functor'

def get_itk_filter_struct(filter_data: FilterData) -> str:
    members = [get_parameter(member).get_itk_functor_member_str(member) for member in filter_data.members]
    members_str = '\n  '.join(members)
    if members:
        members_str = f'\n  {members_str}\n'
    setters = [get_parameter(member).get_itk_functor_body_str(member) for member in filter_data.members]
    setters_str = '\n    '.join(filter(lambda item: item != '', setters))
    if setters:
        setters_str = f'\n    {setters_str}'

    filter_type = f'itk::{filter_data.name}<InputImageT, OutputImageT>'

    if filter_data.filter_type:
        filter_type = filter_data.filter_type
        filter_type = filter_type.replace('InputImageType', 'InputImageT')
        filter_type = filter_type.replace('OutputImageType', 'OutputImageT')

    struct_str = f'''struct {get_itk_functor_name(filter_data)}
{{{members_str}
  template <class InputImageT, class OutputImageT, uint32 Dimension>
  auto createFilter() const
  {{
    using FilterType = {filter_type};
    auto nx_filter = FilterType::New();{setters_str}
    return filter;
  }}
}};'''

    return struct_str

def get_itk_functor_decl(filter_data: FilterData) -> str:
    members = ', '.join([make_first_letter_lowercase(item.name) for item in filter_data.members])
    statement = f'const cx{filter_data.filter_name}::{get_itk_functor_name(filter_data)} itkFunctor = {{{members}}};'
    return statement

def get_preflight_defs(filter_data: FilterData) -> List[str]:
    return [get_parameter(item).get_preflight_def(item) for item in filter_data.members]

def get_execute_decl(filter_data: FilterData) -> str:
    output_type_str = f', cx{filter_data.filter_name}::FilterOutputType' if filter_data.output_pixel_type else ''
    return f'ITK::Execute<cx{filter_data.filter_name}::ArrayOptionsType{output_type_str}>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, itkFunctor, shouldCancel)'

PIXEL_TYPES: Dict[str, str] = {
    'BasicPixelIDTypeList': 'ScalarPixelIdTypeList',
    'ScalarPixelIDTypeList': 'ScalarPixelIdTypeList',
    'IntegerPixelIDTypeList': 'IntegerScalarPixelIdTypeList',
    'RealPixelIDTypeList': 'FloatingScalarPixelIdTypeList',
    'NonLabelPixelIDTypeList': 'ScalarVectorPixelIdTypeList',
    'typelist::Append<BasicPixelIDTypeList, VectorPixelIDTypeList>::Type': 'ScalarVectorPixelIdTypeList',
    'RealVectorPixelIDTypeList': 'FloatingVectorPixelIdTypeList',
    'SignedPixelIDTypeList': 'SignedIntegerScalarPixelIdTypeList',
    'typelist2::append<BasicPixelIDTypeList, VectorPixelIDTypeList>::type': 'UNKNOWN PIXEL TYPE',
    'VectorPixelIDTypeList': 'VectorPixelIDTypeList',
}

INPUT_PIXEL_TYPES: Dict[str, str] = {
    'BasicPixelIDTypeList': 'nx::core::ITK::GetScalarPixelAllowedTypes()',
    'ScalarPixelIDTypeList': 'nx::core::ITK::GetScalarPixelAllowedTypes()',
    'IntegerPixelIDTypeList': 'nx::core::ITK::GetIntegerScalarPixelAllowedTypes()',
    'RealPixelIDTypeList': 'nx::core::ITK::GetFloatingScalarPixelAllowedTypes()',
    'NonLabelPixelIDTypeList': 'nx::core::ITK::GetNonLabelPixelAllowedTypes()',
    'typelist::Append<BasicPixelIDTypeList, VectorPixelIDTypeList>::Type': 'nx::core::ITK::GetScalarVectorPixelAllowedTypes()',
    'RealVectorPixelIDTypeList': 'nx::core::ITK::GetFloatingVectorPixelAllowedTypes()',
    'SignedPixelIDTypeList': 'nx::core::ITK::GetSignedIntegerScalarPixelAllowedTypes()',
    'typelist2::append<BasicPixelIDTypeList, VectorPixelIDTypeList>::type': 'UNKNOWN INPUT PIXEL TYPE'
}

def get_array_options(filter_data: FilterData) -> str:
    code = f'ITK::{PIXEL_TYPES[filter_data.pixel_types]}'
    if filter_data.vector_pixel_types_by_component:
        code += f';\n  //{PIXEL_TYPES[filter_data.vector_pixel_types_by_component]}'
    return code

def get_input_array_types(filter_data: FilterData) -> str:
    return f'{INPUT_PIXEL_TYPES[filter_data.pixel_types]}'


def get_link_ouput_array(filter_data: FilterData) -> str:
    if not filter_data.is_projection:
        return '''  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(outputArrayPath);'''
    return ''

def get_data_check_decl(filter_data: FilterData) -> str:
    filter_data.filter_name
    output_type_str = f', cx{filter_data.filter_name}::FilterOutputType' if filter_data.output_pixel_type else ''
    return f'ITK::DataCheck<cx{filter_data.filter_name}::ArrayOptionsType{output_type_str}>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath)'

def get_output_typedef(filter_data: FilterData) -> str:
    output_type = fixup_type(filter_data.output_pixel_type)

    if output_type == "typename itk::NumericTraits<typename InputImageType::PixelType>::RealType":
        output_type = "double"

    return f'template<class PixelT>\nusing FilterOutputType = {output_type};\n' if output_type else ''

def write_filter_source(filter_data: FilterData, template: Template, output_dir: Path) -> None:
    tag_str = ', '.join([f'\"{tag}\"' for tag in filter_data.default_tags])

    includes = get_parameter_includes(filter_data)
    includes.extend(get_default_parameter_includes())
    includes = remove_duplicates(includes)
    includes = sorted(includes)
    include_str = '\n'.join(includes)
    if includes:
        include_str = f'\n{include_str}'

    itk_filter_include = f'#include <itk{filter_data.name}.h>'

    parameter_defs = get_parameter_defs(filter_data)
    if parameter_defs:
        parameter_defs.insert(0, '  params.insertSeparator(Parameters::Separator{"Input Parameters"});')
 
    parameter_defs_str = '\n'.join(parameter_defs)
    if parameter_defs:
        parameter_defs_str = f'{parameter_defs_str}\n'

    itk_filter_struct = get_itk_filter_struct(filter_data)

    itk_functor_decl = get_itk_functor_decl(filter_data)

    preflight_defs = get_preflight_defs(filter_data)
    preflight_defs_str = '\n  '.join(preflight_defs)
    if preflight_defs:
        preflight_defs_str = f'\n  {preflight_defs_str}'

    execute_decl = get_execute_decl(filter_data)

    array_options = get_array_options(filter_data)

    input_array_types = get_input_array_types(filter_data)

    link_output_array = get_link_ouput_array(filter_data)

    data_check_decl = get_data_check_decl(filter_data)

    output_typedef = get_output_typedef(filter_data)

    substitutions = {
        'FILTER_NAME': filter_data.filter_name,
        'PARAMETER_INCLUDES': include_str,
        'ITK_FILTER_STRUCT': itk_filter_struct,
        'FILTER_HUMAN_NAME': filter_data.human_name,
        'DEFAULT_TAGS': tag_str,
        'PARAMETER_DEFS': parameter_defs_str,
        'PREFLIGHT_DEFS': preflight_defs_str,
        'DATA_CHECK_DECL': data_check_decl,
        'ITK_FUNCTOR_DECL': itk_functor_decl,
        'LINK_OUTPUT_ARRAY': link_output_array,
        'EXECUTE_DECL': execute_decl,
        'ARRAY_OPTIONS': array_options,
        'ITK_FILTER_INCLUDE': itk_filter_include,
        'OUTPUT_TYPEDEF': output_typedef,
        'INPUT_ARRAY_TYPES': input_array_types
    }

    output = template.substitute(substitutions)

    with open(f'{output_dir}/{filter_data.filter_name}.cpp', 'w') as output_file:
        output_file.write(output)

def get_test_case(filter_data: FilterData, test: TestData) -> List[str]:
    lines: List[str] = []

    lines.append(f'TEST_CASE(\"ITKImageProcessing::{filter_data.filter_name}Filter({test.tag})\", \"[{PLUGIN_NAME}][{filter_data.filter_name}][{test.tag}]\")\n')
    lines.append('{\n')
    lines.append('  DataStructure dataStructure;\n')
    lines.append(f'  const {filter_data.filter_name} filter;\n\n')
    lines.append('  const DataPath inputGeometryPath({ITKTestBase::k_ImageGeometryPath});\n')
    lines.append('  const DataPath cellDataPath = inputGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);\n')
    lines.append('  const DataPath inputDataPath = cellDataPath.createChildPath(ITKTestBase::k_InputDataPath);\n')
    lines.append('  const DataObjectNameParameter::ValueType outputArrayName = ITKTestBase::k_OutputDataPath;\n\n')

    for input_str in test.inputs:
        lines.append('  { // Start Image Comparison Scope\n')
        lines.append(f'    const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / unit_test::k_DataDir.view() / \"JSONFilters\" / \"{input_str}\";\n')
        lines.append(f'    Result<> imageReadResult = ITKTestBase::ReadImage(dataStructure, inputFilePath, inputGeometryPath, cellDataPath, inputDataPath);\n')
        lines.append('    SIMPLNX_RESULT_REQUIRE_VALID(imageReadResult)\n')
        lines.append('  } // End Image Comparison Scope\n\n')

    lines.append('  Arguments args;\n')
    lines.append(f'  args.insertOrAssign({filter_data.filter_name}::k_SelectedImageGeomPath_Key, std::make_any<DataPath>(inputGeometryPath));\n')
    lines.append(f'  args.insertOrAssign({filter_data.filter_name}::k_SelectedImageDataPath_Key, std::make_any<DataPath>(inputDataPath));\n')
    lines.append(f'  args.insertOrAssign({filter_data.filter_name}::k_OutputImageDataPath_Key, std::make_any<DataObjectNameParameter::ValueType>(outputArrayName));\n')

    for setting in test.settings:
        param: Parameter = get_parameter_from_name(filter_data, setting.parameter)
        lines.append(f'  args.insertOrAssign({filter_data.filter_name}::{generate_key_var(setting.parameter, False)}, std::make_any<{param.get_type()}>({param.get_test_value_str(setting)}));\n')

    lines.append('\n')
    lines.append('  auto preflightResult = filter.preflight(dataStructure, args);\n')
    lines.append('  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)\n')
    lines.append('\n')
    lines.append('  auto executeResult = filter.execute(dataStructure, args);\n')
    lines.append('  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)\n')
    lines.append('\n')

    if not test.md5hash:
        lines.append(f'  const fs::path baselineFilePath = fs::path(nx::core::unit_test::k_DataDir.view()) / \"JSONFilters/Baseline/BasicFilters_{filter_data.name}_{test.tag}.nrrd\";\n')
        lines.append('  const DataPath baselineGeometryPath({ITKTestBase::k_BaselineGeometryPath});\n')
        lines.append('  const DataPath baseLineCellDataPath = baselineGeometryPath.createChildPath(ITKTestBase::k_ImageCellDataPath);\n')
        lines.append('  const DataPath baselineDataPath = baseLineCellDataPath.createChildPath(ITKTestBase::k_BaselineDataPath);\n')
        lines.append('  const Result<> readBaselineResult = ITKTestBase::ReadImage(dataStructure, baselineFilePath, baselineGeometryPath, baseLineCellDataPath, baselineDataPath);\n')
        tolerance_str = f', {test.tolerance}' if test.tolerance else ''
        lines.append(f'  Result<> compareResult = ITKTestBase::CompareImages(dataStructure, baselineGeometryPath, baselineDataPath, inputGeometryPath, cellDataPath.createChildPath(outputArrayName){tolerance_str});\n')
        lines.append('  SIMPLNX_RESULT_REQUIRE_VALID(compareResult)\n')
    else:
        lines.append('  const std::string md5Hash = ITKTestBase::ComputeMd5Hash(dataStructure, cellDataPath.createChildPath(outputArrayName));\n')
        lines.append(f'  REQUIRE(md5Hash == \"{test.md5hash}\");\n')

    lines.append('}\n')

    return lines

def write_filter_doc(filter_data: FilterData, output_dir: Path) -> None:
    lines: List[str] = []

    lines.append(f'# {filter_data.human_name} ({filter_data.filter_name})\n\n')
    lines.append(f'{filter_data.brief_desc}\n')

    lines.append(f'\n## Group (Subgroup)\n\n')
    lines.append(f'{filter_data.itk_module} ({filter_data.itk_group})\n')

    lines.append(f'\n## Description\n\n')
    lines.append(f'{filter_data.detail_desc}\n')

    # lines.append(f'\n## Misc\n\n')   
    # lines.append(f'filter_data.base_name={filter_data.base_name}\n')
    # lines.append(f'filter_data.default_tag={filter_data.default_tags}\n')
    # lines.append(f'filter_data.filter_name=\n')
    # lines.append(f'filter_data.filter_type={filter_data.filter_type}\n')
    # lines.append(f'filter_data.name={filter_data.name}\n')
    # lines.append(f'filter_data.output_pixel_type={filter_data.output_pixel_type}\n')

    lines.append(f'\n## Parameters\n\n')

    lines.append(f'| Name | Type | Description |\n')
    lines.append(f'|------|------|-------------|\n')
    for i, item in enumerate(filter_data.members):
        lines.append(f'| {item.name} | {item.type} | {item.detaileddescriptionSet} |\n')

    lines.append(f'\n## Required Geometry\n\n')
    lines.append(f'Image Geometry\n')

    lines.append(f'\n## Required Objects\n\n')
    lines.append(f'| Name |Type | Description |\n')
    lines.append(f'|-----|------|-------------|\n')
    lines.append(f'| Input Image Geometry | DataPath | DataPath to the Input Image Geometry |\n')
    lines.append(f'| Input Image Data Array | DataPath | Path to input image with pixel type matching {filter_data.pixel_types} |\n')

    lines.append(f'\n## Created Objects\n\n')
    lines.append(f'| Name |Type | Description |\n')
    lines.append(f'|-----|------|-------------|\n')
    lines.append(f'| Output Image Data Array | DataPath | Path to output image with pixel type matching {filter_data.pixel_types} |\n')


    lines.append(f'\n## Example Pipelines\n\n')

    lines.append(f'\n## License & Copyright\n\n')
    lines.append(f'Please see the description file distributed with this plugin.\n\n')

    lines.append(f'\n## DREAM3D Mailing Lists\n\n')
    lines.append(f'If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:\n')
    lines.append(f'https://groups.google.com/forum/?hl=en#!forum/dream3d-users\n\n')

    lines.append('\n')

    with open(f'{output_dir}/{filter_data.filter_name}.md', 'w') as output_file:
        output_file.writelines(lines)

def write_filter_test(filter_data: FilterData, output_dir: Path):
    output_lines: List[str] = []

    includes = get_parameter_includes(filter_data)

    output_lines.append('#include <catch2/catch.hpp>\n')
    output_lines.append('\n')
    output_lines.append(f'#include \"{PLUGIN_NAME}/Filters/{filter_data.filter_name}.hpp\"')
    output_lines.append('\n')
    output_lines.append('#include \"ITKImageProcessing/Common/sitkCommon.hpp\"')
    output_lines.append('\n')
    output_lines.append('#include "ITKImageProcessing/ITKImageProcessing_test_dirs.hpp"')
    output_lines.append('\n')
    output_lines.append('#include \"ITKTestBase.hpp\"')
    output_lines.append('\n')
    output_lines.append('\n')
    output_lines.append('#include \"simplnx/Parameters/DataObjectNameParameter.hpp\"')
    output_lines.append('\n')
    output_lines.append('#include \"simplnx/UnitTest/UnitTestCommon.hpp\"')
    if includes:
        output_lines.append('\n')
        for i, item in enumerate(includes):
            output_lines.append(item)
            if i != len(includes) -1:
                output_lines.append('\n')
        output_lines.append('\n')
    output_lines.append('\n')
    output_lines.append('\n')
    output_lines.append('#include <filesystem>')
    output_lines.append('\n')
    output_lines.append('namespace fs = std::filesystem;\n')
    output_lines.append('\n')
    output_lines.append('using namespace nx::core;\n')
    output_lines.append('using namespace nx::core::Constants;\n')
    output_lines.append('using namespace nx::core::UnitTest;\n')
    output_lines.append('\n')

    for i, test in enumerate(filter_data.tests):
        test_case_lines = get_test_case(filter_data, test)
        output_lines.extend(test_case_lines)
        if i != len(filter_data.tests) - 1:
            output_lines.append('\n')

    with open(f'{output_dir}/{filter_data.filter_name}Test.cpp', 'w') as output_file:
        output_file.writelines(output_lines)

def write_filter(filter_name: str, json_dir: Path, output_dir: Path, test_output_dir: Path, header_template: Template, source_template: Template, docs_output_dir: Path) -> None:
    filter_data = read_filter_json(json_dir, filter_name)

    # write_filter_header(filter_data, header_template, output_dir)

    # write_filter_source(filter_data, source_template, output_dir)

    # write_filter_test(filter_data, test_output_dir)

    write_filter_doc(filter_data, docs_output_dir)

def main(input_args: Optional[List[str]] = None) -> None:
    parser = argparse.ArgumentParser()

    parser.add_argument('template_dir', type=Path)
    parser.add_argument('json_dir', type=Path)
    parser.add_argument('filter_output_dir', type=Path)
    parser.add_argument('test_output_dir', type=Path)
    parser.add_argument('docs_output_dir', type=Path)

    args = parser.parse_args(input_args)

    template_dir: Path = args.template_dir
    json_dir: Path = args.json_dir
    filter_output_dir: Path = args.filter_output_dir
    test_output_dir: Path = args.test_output_dir
    docs_output_dir: Path = args.docs_output_dir

    filter_output_dir.mkdir(parents=True, exist_ok=True)
    test_output_dir.mkdir(parents=True, exist_ok=True)
    # docs_output_dir.mkdir(parents=True, exist_ok=True)

    with open(f'{template_dir}/filter.hpp.in', 'r') as header_template_file:
        header_template = Template(header_template_file.read())

    with open(f'{template_dir}/filter.cpp.in', 'r') as source_template_file:
        source_template = Template(source_template_file.read())

    filters_to_process: List[str] = [
        'DiscreteGaussianImage',
        'ImageReader',
        'ImageWriter',
        'ImportImageStack',
        'MedianImage',
        'MhaFileReader',
        'RescaleIntensityImage',
        'AbsImage',
        'AcosImage',
        'AdaptiveHistogramEqualizationImage',
        'AsinImage',
        'AtanImage',
        'BinaryContourImage',
        'BinaryDilateImage',
        'BinaryErodeImage',
        'BinaryMorphologicalClosingImage',
        'BinaryMorphologicalOpeningImage',
        'BinaryOpeningByReconstructionImage',
        'BinaryProjectionImage',
        'BinaryThinningImage',
        'BinaryThresholdImage',
        'BlackTopHatImage',
        'ClosingByReconstructionImage',
        'CosImage',
        'DilateObjectMorphologyImage',
        'ErodeObjectMorphologyImage',
        'ExpImage',
        'ExpNegativeImage',
        'GradientMagnitudeImage',
        'GrayscaleDilateImage',
        'GrayscaleErodeImage',
        'GrayscaleFillholeImage',
        'GrayscaleGrindPeakImage',
        'GrayscaleMorphologicalClosingImage',
        'GrayscaleMorphologicalOpeningImage',
        'HConvexImage',
        'HMaximaImage',
        'HMinimaImage',
        'IntensityWindowingImage',
        'InvertIntensityImage',
        'LabelContourImage',
        'Log10Image',
        'LogImage',
        'MaskImage',
        'MorphologicalGradientImage',
        'MorphologicalWatershedImage',
        'NormalizeImage',
        'NotImage',
        'OpeningByReconstructionImage',
        'OtsuMultipleThresholdsImage',
        'RelabelComponentImage',
        'SigmoidImage',
        'SignedMaurerDistanceMapImage',
        'SinImage',
        'SqrtImage',
        'SquareImage',
        'TanImage',
        'ThresholdImage',
        'ValuedRegionalMaximaImage',
        'ValuedRegionalMinimaImage',
        'WhiteTopHatImage'
    ]

    filters_to_skip: List[str] = [
        'BilateralImage', # Vector dimension not supported
        'BinaryClosingByReconstructionImage', # MD5 hash does not match
        'LaplacianSharpeningImage', # Vector dimension not supported
        'MaximumProjectionImage', # Vector dimension not supported
        'MedianProjectionImage', # Vector dimension not supported
        'MinimumProjectionImage', # Vector dimension not supported
        'SaltAndPepperNoiseImage', # Test images out of tolerance
        'ShotNoiseImage',# Test images out of tolerance
        'SpeckleNoiseImage',# Test images out of tolerance

        'BinomialBlurImage',
        'ApproximateSignedDistanceMapImage',
        'BinaryClosingByReconstructionImage',
        'ShiftScaleImage',
        'BinaryDilateImage',
        'BinaryErodeImage',
        'BinaryMorphologicalClosingImage',
        'BinaryMorphologicalOpeningImage',
        'BinaryOpeningByReconstructionImage',
        'BinaryProjectionImage',
        'BinaryThinningImage',
        'BlackTopHatImage',
        'BoundedReciprocalImage',
        'ConnectedComponentImage',
        'CurvatureAnisotropicDiffusionImage',
        'CurvatureFlowImage',
        'DanielssonDistanceMapImage',
        'DilateObjectMorphologyImage',
        'DoubleThresholdImage',
        'ErodeObjectMorphologyImage',
        'ExpImage',
        'ExpNegativeImage',
        'GradientAnisotropicDiffusionImage',
        'GradientMagnitudeRecursiveGaussianImage',
        'GrayscaleDilateImage',
        'GrayscaleErodeImage',
        'GrayscaleGrindPeakImage',
        'GrayscaleMorphologicalClosingImage',
        'GrayscaleMorphologicalOpeningImage',
        'HConvexImage',
        'HMaximaImage',
        'HMinimaImage',
        'IntensityWindowingImage',
        'IsoContourDistanceImage',
        'LabelContourImage',
        'LaplacianRecursiveGaussianImage',
        'LaplacianSharpeningImage',
        'MaximumProjectionImage',
        'MeanProjectionImage',
        'MedianProjectionImage',
        'MinimumProjectionImage',
        'MinMaxCurvatureFlowImage',
        'MorphologicalGradientImage',
        'MorphologicalWatershedFromMarkersImage',
        'MultiScaleHessianBasedObjectnessImage',
        'NormalizeToConstantImage',
        'NotImage',
        'RegionalMaximaImage',
        'RegionalMinimaImage',
        'RelabelComponentImage',
        'SaltAndPepperNoiseImage',
        'SigmoidImage',
        'SignedDanielssonDistanceMapImage',
        'SmoothingRecursiveGaussianImage',
        'StandardDeviationProjectionImage',
        'SumProjectionImage',
        'ThresholdImage',
        'ThresholdMaximumConnectedComponentsImage',
        'ValuedRegionalMaximaImage',
        'ValuedRegionalMinimaImage',
        'WhiteTopHatImage',
        'ZeroCrossingImage',
    ]

    filters_completed: List[str] = [
        'RescaleIntensityImage'
        'DiscreteGaussianImage',
        'BinaryDilateImage',
        'BinaryErodeImage',
        'BinaryMorphologicalOpeningImage',
        'BinaryOpeningByReconstructionImage',
        'BinaryProjectionImage',
        'BinaryThinningImage',
        'BlackTopHatImage',
        'DilateObjectMorphologyImage',
        'ErodeObjectMorphologyImage',
        'ExpImage',
        'ExpNegativeImage',
        'GrayscaleDilateImage',
        'GrayscaleErodeImage',
        'GrayscaleGrindPeakImage',
        'GrayscaleMorphologicalClosingImage',
        'GrayscaleMorphologicalOpeningImage',
        'HConvexImage',
        'HMaximaImage',
        'HMinimaImage',
        'IntensityWindowingImage',
        'LabelContourImage',
        'MorphologicalGradientImage',
        'MultiScaleHessianBasedObjectnessImage',
        'NotImage',
        'RelabelComponentImage',
        'SigmoidImage',
        'SquareImage',
        'ThresholdImage',
        'ValuedRegionalMaximaImage',
        'ValuedRegionalMinimaImage',
        'WhiteTopHatImage'
    ]

    for filter_name in FILTERS:
        if filter_name not in filters_to_process:
            continue
        #print(f'cp /Users/mjackson/DREAM3D-Dev/DREAM3D_Plugins/ITKImageProcessing/Documentation/ITKImageProcessingFilters/ITK{filter_name}.md /Users/mjackson/Workspace1/simplnx/src/Plugins/ITKImageProcessing/docs/.')
        write_filter(filter_name, json_dir, filter_output_dir, test_output_dir, header_template, source_template, docs_output_dir)
    
    # this will print out some code that will need to be included into the ITKImageProcessingPlugin.cpp file
    # for filter_name in FILTERS:
    #     if filter_name not in filters_to_process:
    #         continue
    #     print(f'#include \"ITKImageProcessing/Filters/ITK{filter_name}.hpp\"')

if __name__ == '__main__':
    sys.exit(main())

# python itk_filter_generator.py . SimpleITK/Code/BasicFilters/json output output/test
# python itk_filter_generator.py . SimpleITK/Code/BasicFilters/json D:/Projects/BlueQuartz/temp/complex_plugins/ITKImageProcessing/src/ITKImageProcessing/Filters D:/Projects/BlueQuartz/temp/complex_plugins/ITKImageProcessing/test
