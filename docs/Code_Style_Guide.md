# Code Style Guide #

This is the style guide for the COMPLEX library. When contributing sources to the complex repository we ask that all contributions follow this style guide. These general rules have been developed over the years in an effort to allow efficient coding practices.

## Source Code Line Endings ##

 All source code line endings should be in "Unix" style "\n". Every text editor understands these line endings on every platform **except** the "Notepad" application on Windows.

## Use of Tabs ##

Spaces should be used instead of hard tabs. This helps file portability across different editors. DREAM.3D uses a standard whereby all indents  use two spaces.

## Always Use an Include Guard ##

Always use

    #pragme once

at the top of *EVERY* header file.

## Special Constants ##

nullptr should be used in conjunction with only raw pointers and 0 (zero) should be used in conjunction with numeric values:

    int* value = nullptr; // Good
    int* value = 0; // Bad

    int i = 0; // Good
    int i = NULL; // Bad 

## Array Initialization ##

Prefer the use of C++11 std::array<N, T> instead of "C" style arrays.

    int32_t foo[3]; // BAD.. No initialization of the internal data
    std::array<int32_t, 3> foo = {0,0,0};

## The Dereference operator `*` and the Address-Of Operator `&` Should Be Directly Connected with the Type-Specifier ##

    int32*   p; // Correct
    int32   *p; // Incorrect
    int32*   p, q; // Probably error

The `int32*  p;` form emphasizes type over syntax while the `int32 *p;` form emphasizes syntax over type. Although both forms are equally valid C++, the heavy emphasis on types in C++ suggests that `int32* p;` is the preferable form.

## Always Implement the "Big Five" in C++ Classes ##

When writing C++ classes the programmer will always define the "Big Three" which are defined as:

- copy constructor
- copy assignment operator
- move constructor
- move assignment
- destructor

The programmer should **never** allow the compiler to implement these methods. This will happen if they are not explicitly defined in the class declaration. It is up to the develop to decide if the methods
should be marked as deleted or default'ed or manually implemented.

    class Foo
    {
      public:
        Foo();
        ~Foo();

        Foo(const Foo&) = delete;            // Copy Constructor Not Implemented
        Foo(Foo&&) = delete;                 // Move Constructor Not Implemented
        Foo& operator=(const Foo&) = delete; // Copy Assignment Not Implemented
        Foo& operator=(Foo&&) = delete;      // Move Assignment Not Implemented
    };

Note that with C++11, programmers now have the ability to inform the compiler which of these operations can be constructed by default and which can be ignored using the `default` and `delete` keywords.

## Naming conventions ##

There is an include clang-tidy file that most IDE's can use to assist with some of the major areas. One area is the naming conventions that COMPLEX uses. Here are the basic rules:

- Class and Structs are `UpperCamelCase`
- Class private member variables are `m_UpperCamelCase`
- Class methods are `lowerCamelCase`
- Method arguments are `lowerCamelCase`
- Normal variables are `lowerCamelCase`
- Constants are `k_UpperCamelCase`
- Global statics are `s_UpperCamelCase`
- Free Functions are `UpperCamelCase`
- Macros are `ALL_UPPER_SNAKE_CASE`

## String Constants ##

String constants in C++ should be declared as:

    const std::string MyFile("SomeFile.dat");

Like constants should be grouped into a namespace or anonymous namespace if appropriate

    namespace MyConstants {
        const std::string MyFile("SomeFile");
    }

When using ANSI C one should use a char* for constant strings:

    const char* MyFile "SomeFile.dat";
    const char MyFile[5] = { 'a', '.', 'd', 'a', 't'};

Using this type of approach allows for quicker code updates when constant values need to be changed.

## Clang Formatting ##

`clang-format` is a tool to automatically format C++ code. You should run 'clang-format' on your code before opening a Pull Request to make sure it passes the *clang-format pr* check. You can install `clang-format` and `git-clang-format` with `npm install -g clang-format`. To automatically format a file according to a project's C++ code style, run `clang-format -i path/to/complex/file`, which is supported on macOS/Linux/Windows. If you want to run `clang-format` on all the changed code on your latest git commit (HEAD), you can run `git-clang-format HEAD~1`. Run `git-clang-format -h`  for extra help.
