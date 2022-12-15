# README #

- - -
**NOTE:** This is an import of an old Bitbucket project that was alive back
when Bitbucket supported Mercurial. It is from 2014-2015, and hence does not
consider newer additions to the C++ language. I have reimported it without
history. Since the history really didn't contain that many commits. The old
Mercurial repository can be found [in the Bitbucket archive](https://bitbucket-archive.softwareheritage.org/projects/to/tommyandersen/templated-c-string-format.html).
- - -

Personally I was a bit surprised when the variadic templates were introduced
with C++ 11, that the string formatting methods were kept, in the old
fashioned variable arguments way.  It may come as no surprise to most
developers that the classic variable arguments implementation in C and C++,
leaves much that could be improved upon.  The weaknesses of the existing
approach, is outside the scope of this document, however it exists and is a
real problem unless you are really careful when using the routines.

So when to my surprise the C++ 11 standard was introduced without a much
needed improvement for the string format functions (`sprintf` and the like),
and still in the C++ 14 standards this was not on schedule, I decided to
attempt creating a set of string formatting methods, using variadic templates,
my self.

This is the result of my work, and while it might still at some point be
considered, work-in-progress I find it complete enough for use in production
code.  There are obviously still things that can be improved upon, so far
performance has not been a major focus of mine, although I think the
performance is acceptable.

## WHY VARIADIC TEMPLATES ##

One thing I personally always try to enforce, as much as possible, is to allow
the compiler to do as much of the work as possible.  If possible I try to
bring my self in a situation where, the compiler can do the actual validity
checking.  The variable arguments approach that used to be the foundation for
format like functions like the `printf` family, has a few major flaws, that
opens for a lot of different potential bugs, that may be hard to find, and
unless you understand the finer mechanics behind the variable argument
implementations in C and C++, may be difficult to understand.

As it has been seen before, uncontrolled format strings may even lead to
vulnerabilities of more serious degree as also shown by scut and team seso:
[Exploiting Format String Vulnerabilities](https://crypto.stanford.edu/cs155/papers/formatstring-1.2.pdf).

Until C++11 there was no really feasible alternative to the problematic
formatting strings, alternatives existed, but never really with a intuitive
and light weight interface such as the `printf` family.  But with C++11 came
the introduction of variadic templates, which opened for an exciting
possibility, in true C++ tradition, compile-time checking the data type of
arguments became possible, and allowed the removal of the type specifier in
the formatting string.  No more `%s`, `%d`, and especially `%n` in the format
string, it would now be possible to simply write `%`, or some other clever
code to represent the parameter insertion point in the string.

Variadic templates is really a different approach to many variable argument
approaches where the number of arguments are either pushed on the stack, or
only known by the caller (as is the case in C and C++). Variadic templates
expands into multiple functions, and while this could lead to code bloat, it
also allows for great optimization, and gives new possibilities to a language
like C++.

## HOW TO USE ##

Using the new string formatting methods is relatively straight forward,
include the source file in your project, or pre-build and link.  Simply
include the `format.h` file where needed, and you are set to go.  Below I have
written a really simple example, that shows the basic use of the formatting
methods (of course you can always take a look at the `test.cpp` file, where I
have included more samples).

    #include <iostream>
    #include <utils/format.h>

    using namespace std;
    using namespace utils::str;

    int main(int argc, char* argv[]) {
        cout << Format("Hello {}", "World") << endl;
        return 0;
    }

The above code example will result in the following line being written to
STDOUT:

    Hello World

### New possibilities, new syntax ###

The first thing to notice with the simple example above, is that the
formatting method `Format` uses a different syntax for the formatting string
than the traditional `printf` like functions.  Where the `printf` family of
functions use the `%[options][type]` syntax, the new `Format` method uses a
syntax resembling that of [Python](https://docs.python.org/3.5/library/string.html#format-string-syntax)
or [the .NET langauges](https://msdn.microsoft.com/en-us/library/system.string.format%28v=vs.110%29.aspx?cs-save-lang=1&cs-lang=csharp).
Where the format options are enclosed in `{` and `}`.  In fact the Python
[PEP-3101](https://www.python.org/dev/peps/pep-3101/) specification has been
the inspiration for the syntax used in the new string formatting method.

The string formatting syntax of C and C++ has been around for quite some time,
and an adoption is even usable in Python.  It would seem like it was naturally
mature and well tested, so why bother changing it?

I have decided to use a different syntax for several reasons, although one
might argue, that since a similar syntax is already being used in Python and
.NET and probably several other places as well it would not be fair to phrase
the syntax as new -- and they would be absolutely correct.

I have chosen to use the *"new"* syntax for the following reasons:

1. The syntax is well known in other frameworks and languages, so the cost of
   using this new syntax is limited, since there is a very good chance that
   the developers already know of it, or will find it easy to learn.
2. The old syntax needed to solve a different problem, while it still needed
   to insert data into a text string, it was more focused on data type, rather
   than data presentation; partly due to limitations in the language.
3. With variadic templates came the possibility of extracting the data type
   directly from the argument, so the need for developer to specify the data
   type in the formatting string disappeared.  While it would have been
   possible to keep the data type specification in the format string and
   simply use it for validation, either aborting if the types differed or
   converting, the question would be, would it really add any value? I think
   it would cause more problems than it solved.
4. I wanted to make it easy to refer to arguments by index within the format
   string, this new syntax supported quite easily.
5. I simply liked the new syntax better than any hybrid using the old.

So the syntax chosen is based on the Python PEP-3101 as mentioned earlier,
although it is based on this specification, it is not implemented entirely.
PEP-3101 is quite advanced, and allows for instance nested fields, something
that is currently not supported by my library, whether it will be implemented
at some later point in time is currently unclear.

Besides nested fields, the `c` integer presentation type, converting the
integer number to the corresponding unicode character, is not supported
either. This is currently on my todo list, and I expect it to be implemented
sometime in the future.  There might be other minor issues that differentiate
from PEP-3101 that I have not included here, feel free to share feedback or
problems encountered.

I have made some effort to make the function create output that equals that of
Python's equivalent function, during development, I have had approximately 100
individual test cases run against the corresponding output from Python, and
even discovered som [irregularities](http://bugs.python.org/issue22546) in the
Python documentation on the way.

These tests have even forced me to change the default C++ floating point
outputting methods, since there were differences with how this was handled in
Python.  Floating point outputting is not surprisingly (to me at least) one of
the more complex situations that have required some adaptation.

Obviously a language like Python allows certain features that are difficult to
recreate in C++, for instance referencing class properties by name of
arbitrary types.  I have tried to solve this by allowing the developer to
create mapper functions for these properties, the solution is not entirely
equal or as simple as the one in Python.

Take a look at [PEP-3101](https://www.python.org/dev/peps/pep-3101/) and see
what possibilities the string formatting method offers.

### The basics ###

I have written a brief documentation on how to use some of the core
functionality of the library below, starting from the most simple examples to
the more complex and advanced examples.

#### Simple parameter referencing ####

In the simples form parameters can be referenced by simply using `{}`, the
following is somewhat equivalent to the `printf` string `"%s %s"`, the main
difference with the new syntax is, that the string specifier `s` is not used,
instead the type of the parameter is deduced from the type of the argument. In
other words you no longer accidentally type `"%d"` when you really wish to
insert a string.

    #include <iostream>
    #include <utils/format.h>

    using namespace std;
    using namespace utils::str;

    int main(int argc, char* argv[]) {
        cout << Format("{} {}", "Hello", "World") << endl;
        return 0;
    }

Outputs:

    Hello World

Like with the ordinary `%s` syntax the parameter to insert is automatically
deduced from the order of the fields in the format string. Hence the first
field specifier refers to parameter with index 0 (that is the first parameter
following the format string), while the next field refers to the parameter
with index 1, etc.

Unlike the regular format string method, it is possible to reference
parameters in any order you wish, as can be seen in the example below:

    #include <iostream>
    #include <utils/format.h>

    using namespace std;
    using namespace utils::str;

    int main(int argc, char* argv[]) {
        cout << Format("Countdown from 5: {4}, {3}, {2}, {1}, {0}", 1, 2L, "3", 4, 5) << endl;
        return 0;
    }

Outputs:

    Countdown from 5: 5, 4, 3, 2, 1

Notice how the parameters are of different type, since the parameter holding
the value `"3"` is a null terminated string, while the other 4 numbers are
integers and longs, the format string is still the same though.

It is possible to reference the same parameter multiple times, so a format
string like `"{0} {0} {0}"` would simply result in the first parameter being
inserted three times.

Unlike the Python implementation it is even possible to mix references
specifying the parameter index, and the simple form, where the index is not
specified.  In this case the field not having a set index, will refer to the
index number following the previous field, even if that parameter has been
referenced before.

    #include <iostream>
    #include <utils/format.h>

    using namespace std;
    using namespace utils::str;

    int main(int argc, char* argv[]) {
        cout << Format("March: {} {}, {0} {}, {0} {}", "left", "right") << endl;
        return 0;
    }

Outputs:

    March: left right, left right, left right

#### Formatting specifiers ####

Just like the regular `printf` like formatting string the new formatting
string supports formatting specifiers. It is possible to set width, padding,
etc. But unlike the regular formatting specifiers (but just like the PEP-3101
specifiers) it is possible to set alignment, so a value can be either left,
right, center, or internal aligned.

Formatting specifiers are positioned inside the `{` and `}` just like (but
following) the parameter index. To output a number with a specific precision
set the desired precision as such:

    #include <iostream>
    #include <utils/format.h>

    using namespace std;
    using namespace utils::str;

    int main(int argc, char* argv[]) {
        cout << Format("{0:.0}, {0:.1}, {0:.2}, {0:.3}, {0:.4}", 2.7182818284590452353603L) << endl;
        return 0;
    }

Outputs:

    2, 2.7, 2.72, 2.718, 2.7183

#### Support for vectors and maps and the like ####

The formatting method not only displays primitives, but also regular string
objects can be output, and even vectors and maps are supported.  The syntax
allows to reference specific elements in the vector or map, using either the
object notation (`.`) or the array notation (`[]`), the result is the same.

    #include <iostream>
    #include <map>
    #include <utils/format.h>

    using namespace std;
    using namespace utils::str;

    int main(int argc, char* argv[]) {
        map<string, string> testMap = {{"key1", "value1"}, {"key2", "value2"}, {"key3", "value3"}};
        cout << Format("{0.key1}, {0[key3]}", testMap) << endl;
        return 0;
    }

Outputs:

    value1, value3

As mentioned previously vector like classes can also be used:

    #include <iostream>
    #include <vector>
    #include <utils/format.h>

    using namespace std;
    using namespace utils::str;

    int main(int argc, char* argv[]) {
        vector<int> testVec = {1, 2, 3, 4, 5};
        cout << Format("{0.0}, {0[2]}, {0.4}", testVec) << endl;
        return 0;
    }

Outputs:

    1, 3, 5

#### Extending support to new types ####

Like with Python it is possible to extend the core type support, to allow for
new types to be formatted. Unlike Python however, this is not done through
overriding a format method on a class.  But by implementing a number of
functions that converts and formats the data.  The way this is thought of, is
a lot like the `to_string` method, where new types are supported by
implementing new versions of the function.

New types are supported by implementing the function `FormatType`, which takes
three arguments, the first is the value to format, the second is the format
specifier to format using (the values after the `:` if any).  The last
parameter is the output stream to output to once formatted.

Before `FormatType` is called, the method `ConvertAndFormatType` is called,
this function is called to allow [explicit type conversion flags](https://www.python.org/dev/peps/pep-3101/)
to be used and implemented, if the `ConvertAndFormatType` functions converts
the type to a different type, it is also responsible for calling `FormatType`
on the new type, if not, the function returns `false` and the calling function
inherits that responsibility.

`ConvertAndFormatType` can be implemented for custom types just as well as
`FormatType`, it takes three arguments, the first is the value to format, the
second is a reference to the `FormatFragment` structure, a structure holding
the information needed to format and output the value, and finally the output
stream to output to.  The function returns a boolean, which must be `true` if
the function called `FormatType` and `false` if not.

The function `FormatType` takes a parameter containing the format specifiers
(the second parameter), which is a basic null terminated string, the reason
for this, and not a parsed structure, is that different types, can implement
different format specifier syntaxes (per PEP-3101), so the format specifier
for a date might look like this: `"yyyy-mm-dd HH:MM:SS"` instead of the
regular syntax, if a new type needs to reuse the regular formatting syntax,
the helper function `ConvertToBasicFormatSpecifiers` has been implemented,
this function converts the null terminated string, to a structure of type
`BasicFormatSpecifiers` which holds all relevant information.

#### Special functions (experimental) ####

One last feature that has not been given too much attention, is the parameter
mutator function support. For certain types, some build-in functions will
allow for mutating the value before displaying it. For integers the following
functions are implemented by default:

`abs` Will return the absolute value of the parameter.

`sign` Will return 1 if the value is 0 or greater than zero and -1 if the
value is less than zero.

`inc` Will return the value of the parameter + 1.

`dec` Will return the value of the parameter - 1.

`sqrt` Will return the square root of the value of the parameter.

These special functions can be called multiple times, and are called just like
a selector for a map:

    #include <iostream>
    #include <utils/format.h>

    using namespace std;
    using namespace utils::str;

    int main(int argc, char* argv[]) {
        cout << Format("{0.inc.inc.sqrt}", 7) << endl;
        return 0;
    }

Outputs:

    3

The above example will print `3`, since seven incremented twice is 9, and the
square root of 9 is 3.

## Any other questions? ##

Feel free to ask, but you are always welcome to examine the source code,
chances are the source code documentation might cover it.

Good luck :)

## LICENSE ##

Copyright (c) 2014, Tommy Andersen
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
