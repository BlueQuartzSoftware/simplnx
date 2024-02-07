# Attribute Array Calculator

## Group (Subgroup)

Core (Generation)

## Description

This **Filter** performs calculations on **Attribute Arrays** using the mathematical expression entered by the user, referred to as the *infix expression*. Calculations follow standard mathematical order of operations rules. Parentheses may be used to influence priority. The output of the entered equation is stored as a new **Attribute Array** of type double in an **Attribute Matrix** chosen by the user.

## Usage & Syntax

The user may enter any valid mathematical expression that uses numbers, operators and/or available **Attribute Arrays**.  This expression may be typed into the **Filter** or entered using the available calculator interface. The **Filter** automatically determines how many tuples and component dimensions the output array requires.  Should the entered expression use arrays, computations performed by the **Filter** are performed per tuple, i.e. each tuple has the same expression performed. Therefore, any **Attribute Arrays** used in the entered expression must have the same number of tuples. To help prevent most cases of tuple incompatibilities, the user must select an **Attribute Matrix** to serve as the source for arrays to be used in the expression. Additionally, the output array will have the same number of tuples as the arrays used in the infix expression, and must be placed in an **Attribute Matrix** that has the same number of tuples as the source **Attribute Matrix**.

All items in the entered infix expression, including values within arrays, will be cast to doubles for computation, and the resulting output will be stored as doubles. If the output array needs to be a different type for use as input to another **Filter**, consider using the Convert Attribute Data Type **Filter**.

### Expressions Without Arrays

It is possible to enter an infix expression that does not contain any **Attribute Array**, similar to a standard calculator. In this case, the output array is simply a single numeric value that is stored in a single component, one tuple array. Because the output array will only have one tuple, it must be placed in an **Attribute Matrix** that has exactly one tuple.  If such an **Attribute Matrix** is not available in the data structure, it can be created using the Create Attribute Matrix **Filter**.

### Operators

There are several mathematical operators available for usage in an infix expression. Operators may either be *binary* (requiring two items to operate) or *unary* (requiring only one item to operate). Some unary operators may also require more than one argument.  The tables below list the available mathematical operators.

#### Binary Operators

| Symbol | Name | Operation |
|-----------|----------|---------------|
| `+` | addition | Adds two items |
| `-` | subtraction | Subtracts two items |
| `*` | multiplication | Multiplies two items |
| `/` | division | Divides two items |
| x^y | exponentiation | Raises the base *x* to the *y* power |

#### Unary Operators

| Symbol | Name | Number of Arguments | Operation |
|-----------|----------|-------------------------------|---------------|
| e^y | exponential function | 1 | Raises *e* to the *y* power |
| sin(x) | sine function | 1 | Computes the sine of *x* |
| cos(x) | cosine function | 1 | Computes the cosine of *x* |
| tan(x) | tangent function | 1 | Computes the tangent of *x* |
| asin(x) | arcsine function | 1 | Computes the arcsine of *x* |
| acos(x) | arccosine function | 1 | Computes the arccosine of *x* |
| atan(x) | arctangent function | 1 | Computes the arctangent of *x* |
| abs(x) | absolute value | 1 | Returns the absolute value of *x* |
| ceil(x) | ceiling function | 1 | Returns the smallest integral value not less than *x* |
| floor(x) | floor function | 1 | Returns the largest integral value not greater than *x* |
| log10(x) | base 10 logarithm | 1 | Computes the base 10 logarithm of *x* |
| ln(x) | natural logarithm | 1 | Computes the natural logarithm of *x* |
| sqrt(x) | square root | 1 | Computes the square root of *x* |
| log(b, x) | base *b* logarithm | 2 | Computes the base *b* logarithm of *x* |
| root(r, ind) | *ind<sup>th</sup>* root | 2 | Computes the *ind<sup>th</sup>* root of the radicand*r* |

#### Trigonometric Operators and Degrees/Radians

The direct trigonometric operators (**sin**, **cos** and **tan**) can operate on either radians or degrees, which can be selected by the user from the **Filter** interface.  Similarly, the inverse trigonometric operators (**asin**, **acos** and **atan**) will return either radians or degrees depending on the selection in the **Filter** interface. Note that by default, DREAM.3D **Filters** generally assume angle values are in radians. The Convert Angles to Degrees or Radians can be used to convert arrays from radians to degrees and vice versa.

#### Undefined Operations and Operators Out-Of-Range

This **Filter** allows for undefined operations to occur. The return values for these operations will be the same return value obtained from the utilized C/C++ function. For example, the divide by zero operation in C/C++ is undefined behavior, so the result is not guaranteed (however, most platforms will follow the IEEE floating point standard and a value of +infinity will be returned). Similarly, a domain error may occur if an operator function is used with arguments out of range. For example, entering `arcsin(-2)`, `ln(-7.34)` or `sqrt(-14.89)` results in a domain error. The return value here is again not guaranteed, but would most likely be NaN.

### Using Arrays in Expressions

Any **Attribute Array** available in the selected **Attribute Matrix** is eligible to be used in the infix expression and multiple arrays may be used in the same expression, but all arrays in any given expression must have the same number of components.  Arrays may be entered into the infix expression by typing their exact name, or by using the dropdown menus in the interface labeled *Scalars* (for single component arrays) or *Vectors* (for multi-component arrays). Operations on arrays are computed tuple-by-tuple. For example, consider a single component **Attribute Array** named *Foo* that has five tuples, with the values stored being the first 5 prime numbers. The array *Foo* then looks as follows:

    [2, 3, 5, 7, 11]

If the user enters the infix expression `Foo + 3`, the result of the **Filter** is a new array with exactly five tuples that contains the values:

    [5, 6, 8, 10, 14]

Now consider another single component array named *Bar* that has the value 4.25 at all tuples.  If the infix expression `(Foo + Bar) * 4` is entered, then the output values are as follows:

    [25, 29, 37, 45, 61]

If there is a third available array named *Baz* that has three components, it cannot be used in infix expressions with *Foo* or *Bar*. For example, the infix expression `Foo + Baz` is not valid, and a parsing error will be presented.

#### Using Multi-Component Arrays

There are two ways to enter multi-component arrays into the infix expression.  Arrays may be entered without an index for a component. In this case, operations are performed both tuple-by-tuple *and* component-by-component, and the output array will have the same number of components as the input arrays. For example, consider two arrays *Foo* and *Bar* that are each 3-component arrays with two tuples whose values are as follows:

    Foo: [{1, 2, 3}, {4, 5, 6}]
    Bar: [{-1, -2, -3}, {-4, -5, -6}]

If the infix expression `Foo + Bar` then the output values are as follows:

    [{0, 0, 0}, {0, 0, 0}]

Alternatively, multi-component arrays may be entered specifying a particular *index*. In this case, operations are performed tuple-by-tuple, but only for the specific component specified in each array, so the output array will always be a scalar value. Indices are specified using `[]` notation after the array name. Additionally, indices are *zero based*.  For example, `Foo[1]` is valid syntax, but `Foo[3]` is not valid. Additionally, indices must be positive integers only.  If any array in the infix expression uses index notation, then all arrays must specify an index.  

Consider the infix expression `Foo[0] + Foo[1] + Foo[2]`. This expression will add all the components of *Foo* for each tuple. The result of this expression is the following:

    [6, 15]

The infix expression `Foo[0] * Bar[2]` multiplies the first component of *Foo* with the third component of *Bar* for each tuple.  The result if this expression is as follows:

    [-3, -24]

#### Explicit Array Names

It is possible to force the **Filter** to treat a term in the infix expression as the literal name of an **Attribute Array**.  To force an item to be considered an array name literal, surround it in double quotes. Forcing literal names in this fashion allows for arrays to take the same name as mathematical operators. Consider the following examples:

1. If there is an array named "+", the user can create a valid equation such as `3 + "+"`.  This equation adds three to every element in the "+" array.

2. For an array names "452", the user can create a valid equation such as `6 * "452"`.  This equation multiplies every element in the "452" array by 6.

3. For array names "-" and "4" with 3 components each, the user can create a valid equation such as `"-"[2] + "4"[0]`.  This equation adds the third component in array "-" to the first component in array "4" for each tuple.

In most cases, attempting to write equations like these without putting the double quotes around the desired array name will cause an error due to ambiguity.

## Examples

    Confidence Index + 3 / ln(4*Fit)

This equation multiplies every tuple in the *Fit* array by *4* and then finds the *natural logarithm* of those results.  Then, it divides *3* by those values and *adds* the results of that to every element in the *Confidence Index* array.

    Array1[0] - Array2[1]

This equation subtracts the second component in *Array2* from the first component in *Array1* for each tuple. The resulting array will have the same number of tuples as *Array1* and *Array2*, but only one component.

    Array1 + Array2

This equation adds every element in *Array2* with every element in *Array1*.  The resulting array will have the same number of tuples and components as *Array1* and *Array2*.

    6 / 3

This equation divides *6* by *3* and stores the result in an array with exactly one component and one tuple.

    root(8, 3)

This equation takes the cube root of *8* and stores the result in an array with exactly one component and one tuple.

    3 ^ Array1

This equation raises *3* to the power of the value stored in every component of every tuple in *Array1*.  The resulting array has the same tuple and component size of *Array1*.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
