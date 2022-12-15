#ifndef UTILS_STR_FORMAT_H_
#define UTILS_STR_FORMAT_H_

/*

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

*/

#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <list>
#include <queue>

// The macro FORMAT_DISABLE_THROW_OUT_OF_RANGE will if defined disable throwing of exceptions if an argument
// referenced in the format string, is not passed as parameter.  This line is intentionally commented out, to document
// its existence, while not enabling it.
// #define FORMAT_DISABLE_THROW_OUT_OF_RANGE 1

/**
 * The data to write before the first element of an array.
 */
#ifndef FORMAT_ARRAY_OPEN
#  define FORMAT_ARRAY_OPEN "["
#endif

/**
 * The data to write after the last element of an array.
 */
#ifndef FORMAT_ARRAY_CLOSE
#  define FORMAT_ARRAY_CLOSE "]"
#endif

/**
 * The data to write between each element of an array.
 */
#ifndef FORMAT_ARRAY_SEP
#  define FORMAT_ARRAY_SEP ", "
#endif

/**
 * The data to write before the first element of a map.
 */
#ifndef FORMAT_MAP_OPEN
#  define FORMAT_MAP_OPEN "{"
#endif

/**
 * The data to write after the last element of a map.
 */
#ifndef FORMAT_MAP_CLOSE
#  define FORMAT_MAP_CLOSE "}"
#endif

/**
 * The data to write between each key-pair value in a map.
 */
#ifndef FORMAT_MAP_SEP
#  define FORMAT_MAP_SEP ", "
#endif

/**
 * The data to write before the first element of a pair.
 */
#ifndef FORMAT_PAIR_OPEN
#  define FORMAT_PAIR_OPEN ""
#endif

/**
 * The data to write after the last element of a pair.
 */
#ifndef FORMAT_PAIR_CLOSE
#  define FORMAT_PAIR_CLOSE ""
#endif

/**
 * The data to separate the first and last element of a pair.
 */
#ifndef FORMAT_PAIR_SEP
#  define FORMAT_PAIR_SEP ": "
#endif

namespace utils {
namespace str {

//
// Error-handling
//

/**
 * The IllegalFormatStringException is a runtime exception thrown when a format string is syntactically wrong, or if
 * any errors occurred while processing the format string, such as integer overflows.
 */
class IllegalFormatStringException
    : public std::runtime_error
{
public:
    /**
     * The constructor, constructing an IllegalFormatStringException for a specific format string, for an error
     * occurring at position @p pos with the error message: @p message
     *
     * @param[in] formatStr  The format string causing the exception to be thrown.
     * @param[in] pos  The position in the format string where the problem was detected.
     * @param[in] message  The message describing the problem found.
     */
    IllegalFormatStringException(const std::string& formatStr, int pos, const std::string& message);
    virtual ~IllegalFormatStringException() noexcept;

    /**
     * Returns the position in the format string where the error was detected.
     *
     * The position returned i 0 based, so a value of 0 indicates the first character in the format string.
     *
     * @return Returns the position in the format string where the error was detected.
     */
    int GetErrorPosition() const noexcept;

    /**
     * Returns the format string that is illegal.
     *
     * @return Returns the format string that is illegal.
     */
    const char* GetFormatString() const noexcept;

    /**
     * Returns the error message describing the error occurring from parsing the format string.
     *
     * @return Returns the error message describing the error occurring from parsing the format string.
     */
    const char* GetMessage() const noexcept;

    /**
     * Returns a complete descriptive string about the error occurred.
     *
     * @return Returns a complete descriptive string about the error occurred.
     */
    virtual const char* what() const noexcept;

private:
    std::string formatString;
    std::string message;
    std::string fullDescription;
    int errorPosition;
};


//
// Data container structs
//

/**
 * A format fragment describes an entry in the format string beginning with { and ending with }, or it describes a
 * string fragment.
 */
struct FormatFragment
{
    /**
     * If this is a text fragment, the text property is the actual string to output, if this is a format parameter
     * the text property will hold the actual formatted output to write, after the formatting has run.
     */
    std::string text;

    /**
     * If the format specifier is set for format fragments only, this property is not used for text fragments. The
     * contents of this property is the value written as format specifier, that is the text following the : in the
     * format notation, up to, but not including, the closing }. Since different types can have different formatting
     * specifiers, the string is not stored in its parsed form.
     */
    std::string formatSpecifier;

    /**
     * A queue of selectors, selectors are sub references to a parameter, it can for instance be an array index, or
     * map key, or even a special function, or object property.  Several selectors can exist for one format parameter,
     * and if several selectors exist, they are executed in FIFO order, meaning that the first selector is used, and
     * afterwards the second selector is used on the result of the first, and so on.
     */
    std::queue<std::string> selectors;

    /**
     * The index this format fragment points to, if this is 0 or more it is the parameter index to use for this
     * formatting specifier. If this is -1 this struct is a text fragment.
     */
    int index;

    /**
     * Indicates whether the parameter should be explicitly converted to a different type before being formatted, the
     * standard two from Python r and s will both convert the value to string, they are considered equal here, though
     * they are different in Python. The explicity conversion character is specified by using !c where c is the type
     * to convert to. This can be one of: r, s, i, d. Where r and s are strings, i is integer and d is decimal. Note
     * that i and d are unknown in Python, but included here.
     */
    char explicitConversion;

#ifndef FORMAT_DISABLE_THROW_OUT_OF_RANGE
    /**
     * Once a fragment is handled this boolean value will be set to true, if by the end of the processing there are
     * parameters remaining that are not processed and the macro FORMAT_DISABLE_THROW_OUT_OF_RANGE is not defined, an
     * out_of_range exception is thrown.
     */
    bool handled;
#endif  // FORMAT_DISABLE_THROW_OUT_OF_RANGE
};

/**
 * This structure holds basic formatting options used, primarily, for primitive types.
 */
struct BasicFormatSpecifiers
{
    /**
     * @c width is a decimal integer defining the minimum field width. If not specified (or 0), then the field width
     * will be determined by the content.
     */
    int width;

    /**
     * The @c precision is a decimal number indicating how many digits should be displayed after the decimal point in
     * a floating point conversion. For non-numeric types the field indicates the maximum field size - in other words,
     * how many characters will be used from the field content. The precision is ignored for integer conversions.
     */
    int precision;

    /**
     * The @c fill character defines the character to be used to pad the field to the minimum width. The fill
     * character, if present, must be followed by an alignment flag.
     */
    char fill;

    /**
     * The @c align character can be one of the following:
     *
     * @arg @c '<' Forces the field to be left-aligned within the available space (This is the default.)
     * @arg @c '>' Forces the field to be right-aligned within the available space.
     * @arg @c '=' Forces the padding to be placed after the sign (if any) but before the digits. This is used for
     *             printing fields in the form '+000000120'. This alignment option is only valid for numeric types.
     * @arg @c '^' Forces the field to be centered within the available space.
     *
     * Note that unless a minimum field width is defined, the field width will always be the same size as the data to
     * fill it, so that the alignment option has no meaning in this case.
     */
    char align;

    /**
     * The @c sign option is only valid for numeric types, and can be one of the following:
     *
     * @arg @c '+' Indicates that a sign should be used for both positive as well as negative numbers.
     * @arg @c '-' Indicates that a sign should be used only for negative numbers (this is the default behavior).
     * @arg @c ' ' Indicates that a leading space should be used on positive numbers.
     */
    char sign;

    /**
     * The @c type determines how the data should be presented.
     *
     * The available integer presentation types are:
     *
     * @arg @c 'b' Binary. Outputs the number in base 2.
     * @arg @c 'c' Character. Converts the integer to the corresponding Unicode character before printing. This option
     *             has been left out for now.
     * @arg @c 'd' Decimal Integer. Outputs the number in base 10.
     * @arg @c 'o' Octal format. Outputs the number in base 8.
     * @arg @c 'x' Hex format. Outputs the number in base 16, using lower-case letters for the digits above 9.
     * @arg @c 'X' Hex format. Outputs the number in base 16, using upper-case letters for the digits above 9.
     * @arg @c 'n' Number. This is the same as 'd', except that it uses the current locale setting to insert the
     *             appropriate number separator characters.
     * @arg @c ''  (None) - the same as 'd'.
     *
     * The available floating point presentation types are:
     *
     * @arg @c 'e' Exponent notation. Prints the number in scientific notation using the letter 'e' to indicate the
     *             exponent.
     * @arg @c 'E' Exponent notation. Same as 'e' except it converts the number to upper-case.
     * @arg @c 'f' Fixed point. Displays the number as a fixed-point number.
     * @arg @c 'F' Fixed point. Same as 'f' except it converts the number to upper-case.
     * @arg @c 'g' General format. This prints the number as a fixed-point number, unless the number is too large, in
     *             which case it switches to 'e' exponent notation.
     * @arg @c 'G' General format. Same as 'g' except switches to 'E' if the number gets to large.
     * @arg @c 'n' Number. This is the same as 'g', except that it uses the current locale setting to insert the
     *             appropriate number separator characters.
     * @arg @c '\%' Percentage. Multiplies the number by 100 and displays in fixed ('f') format, followed by a percent
     *             sign.
     * @arg @c ''  (None) - similar to 'g', except that it prints at least one digit after the decimal point.
     */
    char type;

    /**
     * If the @c alternateForm is set, integers use the 'alternate form' for formatting. This means that binary,
     * octal, and hexadecimal output will be prefixed with '0b', '0o', and '0x', respectively.
     */
    bool alternateForm;

    /**
     * The @c thousandSeparator signals the use of a comma for a thousands separator. For a locale aware separator,
     * use the 'n' integer presentation type instead.
     */
    bool thousandSeparator;
};


//
// Helper templates
//

namespace helper {

/**
 * A helper struct used to hold a constant boolean value, passed through by a template parameter.
 *
 * @tparam Result The boolean value that should be stored in the value member of this struct.
 */
template<bool Result>
struct Answer
{
    static constexpr bool value = Result;
};


/**
 * A compile time check, made to find out whether a specific type has a child type called iterator or not.
 *
 * If the type referenced by template parameter T has a subclass named iterator we assume this to be a valid iterator
 * type and considers this class to have an iterator.  We take advantage of the SFINAE (Substituion Failure Is Not An
 * Error, acronym first mention in C++ Templates: The Complete Guide) language feature to figure this out.  The struct
 * has a static constant boolean member called value, which is true if the type T is considered to have an iterator or
 * false if not.
 *
 * @note This is not a bulletproof method, a more safe method would be to look for the begin and end methods for the
 * type, since it is these that are required.  However to limit code and not make the template voodoo too extensive
 * the assumption is made that a subclass named iterator really is an iterator and complies with the requirements of
 * the ForwardIterator (http://en.cppreference.com/w/cpp/concept/ForwardIterator) concept.  If one creates a subclass
 * called iterator that is not an iterator, then we have an entirely different problem, and we really should talk.
 *
 * @tparam T The type to test.
 *
 * @see IsMapType
 * @see IsPairType
 * @see Answer
 */
template <typename T>
struct HasIterator
{
    template <typename C>
    static constexpr Answer<true> TestType(typename C::iterator* x);

    template <typename C>
    static constexpr Answer<false> TestType(C* x);

    static constexpr bool value = decltype(TestType<T>(nullptr))::value;
};

/**
 * A compile time check, made to find out whether a specific type has a child type called mapped_type or not.
 *
 * Traditional maps in C++ (std::map, std::unordered_map, and std::multimap) have a child type called mapped_type
 * which is essentially the type of the value the map holds.  That is not the key, value pair but the value part of
 * the pair which is often used in maps.  Like the HasIterator struct this struct tests the type provided by the
 * template parameter using the SFINAE feature of the language.  The struct has a static constant boolean member
 * called value, which is true if the type T has a child type named mapped_type or false if not.
 *
 * @tparam T The type to test.
 *
 * @see HasIterator
 * @see IsPairType
 * @see Answer
 */
template <typename T>
struct IsMapType
{
    template <typename C>
    static constexpr Answer<true> TestType(typename C::mapped_type* x);

    template <typename C>
    static constexpr Answer<false> TestType(C* x);

    static constexpr bool value = decltype(TestType<T>(nullptr))::value;
};


template <typename T, typename K>
struct MapHasKeyType
{
    template <typename C>
    static constexpr Answer<true>    TestType(typename C::key_type* x);

    template <typename C>
    static constexpr Answer<false> TestType(C* x);

    template <typename C>
    static constexpr Answer<false> TestType(void* x);

    static constexpr bool value = decltype(TestType<T>(static_cast<K*>(nullptr)))::value;
};


/**
 * A compile time check, made to find out whether a specific type has both child types: first_type and second_type or
 * not.
 *
 * A pair which holds two values of arbitrary type, the type of the values are stored in the types first_type and
 * second_type, both of which are child types of the pair class.  This knowledge is used to detect whether this class
 * is a pair or compatible or not.  To include possible boost alternatives we use this method of detecting pairs
 * rather than simply overloading.  To be absolutely sure, we require both first_type and second_type to exist.
 *
 * Like the HasIterator struct this struct tests the type provided by the template parameter using the SFINAE feature
 * of the language.  The struct has a static constant boolean member called value, which is true if the type T has a
 * child type named first_type and a child type named second_type, both must exist, or false if not.
 *
 * @tparam T The type to test.
 *
 * @see HasIterator
 * @see IsMapType
 * @see Answer
 */
template <typename T>
struct IsPairType
{
    template <typename C>
    static constexpr Answer<true> TestFirstType(typename C::first_type* x);

    template <typename C>
    static constexpr Answer<true> TestSecondType(typename C::second_type* x);

    template <typename C>
    static constexpr Answer<false> TestFirstType(C* x);

    template <typename C>
    static constexpr Answer<false> TestSecondType(C* x);

    static constexpr bool value = decltype(TestFirstType<T>(nullptr))::value && decltype(TestSecondType<T>(nullptr))::value;
};

}

//
// Conversion functions
//

/**
 * Converts the format parameter string to a struct of basic format specifiers.
 *
 * The format specifiers are extracted from the @p formatParameter argument, this function expects the input string to
 * follow the convention written in Python PEP-3101: https://www.python.org/dev/peps/pep-3101/ Which is the format
 * used for primitive types.
 *
 * @param[in]     formatParameter  The format parameter string
 * @param[out]    specifiers  The specifiers struct to store the result in
 * @param[in,out] pos  The position to begin extracting from, when this function returns this will be updated to the
 *                first position after the format specifier string.
 */
void ConvertToBasicFormatSpecifiers(const char* formatParameter, BasicFormatSpecifiers& specifiers, int& pos);


//
// Formatting functions
// Built-in decimal type conversion
//

/**
 * Formatting function for the primitive type short, this will be called by the format function and can be called as
 * is to format an integer from a specific format specifier.
 *
 * To the formatter all numbers of the type: int, long, and int64 are treated equally, thus this function converts
 * @p value to an int64 and calls the corresponding FormatType function.
 *
 * The format of the format specifier string can be found in Python PEP-3101, where the format specifier is described
 * https://www.python.org/dev/peps/pep-3101/
 *
 * @param value[in]  The value to format.
 * @param formatSpecifier[in]  The format specifier to use.
 * @param output[out]  A reference to an output stream to write the formatted result to.
 */
void FormatType(short value, const char* formatSpecifier, std::ostream& output);

/**
 * Formatting function for the primitive type int, this will be called by the format function and can be called as is
 * to format an integer from a specific format specifier.
 *
 * To the formatter all numbers of the type: int, long, and int64 are treated equally, thus this function converts
 * @p value to an int64 and calls the corresponding FormatType function.
 *
 * The format of the format specifier string can be found in Python PEP-3101, where the format specifier is described
 * https://www.python.org/dev/peps/pep-3101/
 *
 * @param value[in]  The value to format.
 * @param formatSpecifier[in]  The format specifier to use.
 * @param output[out]  A reference to an output stream to write the formatted result to.
 */
void FormatType(int value, const char* formatSpecifier, std::ostream& output);

/**
 * Formatting function for the primitive type long, this will be called by the format function and can be called as is
 * to format an integer from a specific format specifier.
 *
 * To the formatter all numbers of the type: int, long, and int64 are treated equally, thus the corresponding format
 * functions converts their respective @p value to an int64 and calls this function.
 *
 * The format of the format specifier string can be found in Python PEP-3101, where the format specifier is described
 * https://www.python.org/dev/peps/pep-3101/
 *
 * @param value[in]  The value to format.
 * @param formatSpecifier[in]  The format specifier to use.
 * @param output[out]  A reference to an output stream to write the formatted result to.
 */
void FormatType(int64_t value, const char* formatSpecifier, std::ostream& output);

/**
 * Formatting function for the primitive type float, this will be called by the format function and can be called as
 * is to format an integer from a specific format specifier.
 *
 * To the formatter all numbers of the type: float, double and long double are treated equally, thus this function
 * converts @p value to a long double and calls the corresponding FormatType function.
 *
 * The format of the format specifier string can be found in Python PEP-3101, where the format specifier is described
 * https://www.python.org/dev/peps/pep-3101/
 *
 * @param value[in]  The value to format.
 * @param formatSpecifier[in]  The format specifier to use.
 * @param output[out]  A reference to an output stream to write the formatted result to.
 */
void FormatType(float value, const char* formatSpecifier, std::ostream& output);

/**
 * Formatting function for the primitive type double, this will be called by the format function and can be called as
 * is to format an integer from a specific format specifier.
 *
 * To the formatter all numbers of the type: float, double and long double are treated equally, thus this function
 * converts @p value to a long double and calls the corresponding FormatType function.
 *
 * The format of the format specifier string can be found in Python PEP-3101, where the format specifier is described
 * https://www.python.org/dev/peps/pep-3101/
 *
 * @param value[in]  The value to format.
 * @param formatSpecifier[in]  The format specifier to use.
 * @param output[out]  A reference to an output stream to write the formatted result to.
 */
void FormatType(double value, const char* formatSpecifier, std::ostream& output);


/**
 * Formatting function for the primitive type double, this will be called by the format function and can be called as
 * is to format an integer from a specific format specifier.
 *
 * To the formatter all numbers of the type: float, double and long double are treated equally, thus the corresponding
 * format functions converts their respective @p value to a long double and calls this function.
 *
 * The format of the format specifier string can be found in Python PEP-3101, where the format specifier is described
 * https://www.python.org/dev/peps/pep-3101/
 *
 * @param value[in]  The value to format.
 * @param formatSpecifier[in]  The format specifier to use.
 * @param output[out]  A reference to an output stream to write the formatted result to.
 */
void FormatType(long double value, const char* formatSpecifier, std::ostream& output);

// Built-in enum style type conversion
void FormatType(bool value, const char* formatSpecifier, std::ostream& output);

// Built-in string type conversion
void FormatType(const char* value, const char* formatSpecifier, std::ostream& output);
void FormatType(const std::string& value, const char* formatSpecifier, std::ostream& output);

//
// Prototypes
//

template <typename T>
typename std::enable_if<helper::HasIterator<T>::value && !helper::IsMapType<T>::value, void>::type
FormatType(const T& container, const char* formatSpecifier, std::ostream& output);

template <typename T>
typename std::enable_if<helper::HasIterator<T>::value && helper::IsMapType<T>::value, void>::type
FormatType(const T& container, const char* formatSpecifier, std::ostream& output);

template <typename T>
typename std::enable_if<helper::IsPairType<T>::value, void>::type
FormatType(const T& p, const char* formatSpecifier, std::ostream& output);

template <typename T>
typename std::enable_if<helper::HasIterator<T>::value && !helper::IsMapType<T>::value, void>::type
FormatType(const T& container, const char* formatSpecifier, std::ostream& output)
{
    bool first = true;
    output << FORMAT_ARRAY_OPEN;
    for (const auto& elem : container) {
        if (!first) {
            output << FORMAT_ARRAY_SEP;
        }
        FormatType(elem, formatSpecifier, output);
        first = false;
    }
    output << FORMAT_ARRAY_CLOSE;
}

template <typename T>
typename std::enable_if<helper::HasIterator<T>::value && helper::IsMapType<T>::value, void>::type
FormatType(const T& container, const char* formatSpecifier, std::ostream& output)
{
    bool first = true;
    output << FORMAT_MAP_OPEN;
    for (const auto& elem : container) {
        if (!first) {
            output << FORMAT_MAP_SEP;
        }
        FormatType(elem, formatSpecifier, output);
        first = false;
    }
    output << FORMAT_MAP_CLOSE;
}

template <typename T>
typename std::enable_if<helper::IsPairType<T>::value, void>::type
FormatType(const T& p, const char* formatSpecifier, std::ostream& output)
{
    output << FORMAT_PAIR_OPEN;
    FormatType(p.first, formatSpecifier, output);
    output << FORMAT_PAIR_SEP;
    FormatType(p.second, formatSpecifier, output);
    output << FORMAT_PAIR_CLOSE;
}

template <typename T>
void FormatType(const T& value, const std::string& formatSpecifier, std::ostream& output)
{
    FormatType(value, formatSpecifier.c_str(), output);
}

template <typename T>
std::string FormatType(T&& value, const char* formatSpecifier)
{
    std::stringstream buffer;
    FormatType(value, formatSpecifier, buffer);
    return buffer.str();
}

bool ConvertAndFormatType(short value, FormatFragment& fragment, std::ostream& output);
bool ConvertAndFormatType(int value, FormatFragment& fragment, std::ostream& output);
bool ConvertAndFormatType(int64_t value, FormatFragment& fragment, std::ostream& output);
bool ConvertAndFormatType(float value, FormatFragment& fragment, std::ostream& output);
bool ConvertAndFormatType(double value, FormatFragment& fragment, std::ostream& output);
bool ConvertAndFormatType(long double value, FormatFragment& fragment, std::ostream& output);
bool ConvertAndFormatType(bool value, FormatFragment& fragment, std::ostream& output);
bool ConvertAndFormatType(const char* value, FormatFragment& fragment, std::ostream& output);
bool ConvertAndFormatType(const std::string& value, FormatFragment& fragment, std::ostream& output);

template <typename T>
typename std::enable_if<helper::MapHasKeyType<T, std::string>::value, bool>::type
ConvertAndFormatType(const T& value, FormatFragment& fragment, std::ostream& output)
{
    if (!fragment.selectors.empty()) {
        std::string selector = fragment.selectors.front();
        fragment.selectors.pop();

        if (value.count(selector) > 0) {
            ConvertAndFormatType(value.at(selector), fragment, output);
            return true;
        }
    }

    FormatType(value, fragment.formatSpecifier, output);
    return true;
}

template <typename T>
typename std::enable_if<!helper::MapHasKeyType<T, std::string>::value, bool>::type
ConvertAndFormatType(const T&, FormatFragment&, std::ostream&)
{
    return false;
}


//
// Parsing functions
//

/**
 * Parses the format string provided, splitting into segments of either format fragments or text fragments, the actual
 * result string is not fully constructed in this function.
 *
 * When this function parses the string provided by @p formatStr it starts by sending any initial text fragment 
 * beginning the format string directly to the output stream @p ostr.  If the string contains no formatting, the 
 * resulting string will exist in the output stream directly.
 *
 * @param[in]  formatStr This parameter contains a pointer to where parsing of the format string should start.
 * @param[out] ostr This is the output stream to write the result to.
 * @param[out] fragments A vector containing the output fragments extracted by parsing @p formatStr.
 */
void ParseFormatStr(const char* formatStr, std::ostream& ostr, std::vector<FormatFragment>& fragments);

template <int ArgumentIndex, typename T>
void FormatParameter(std::vector<FormatFragment>& fragments, const T& arg)
{
    // Find all fragments that match this index and format them individually.
    for (FormatFragment& fragment : fragments) {
        if (fragment.index == ArgumentIndex) {
            std::stringstream buffer;
            bool isHandled = ConvertAndFormatType(arg, fragment, buffer);
            if (!isHandled) {
                FormatType(arg, fragment.formatSpecifier, buffer);
            }
            // Set the text to use
            fragment.text = buffer.str();

#ifndef FORMAT_DISABLE_THROW_OUT_OF_RANGE
            // Remember that we handled this fragment.
            fragment.handled = true;
#endif  // FORMAT_DISABLE_THROW_OUT_OF_RANGE
        }
    }
}

/**
 * This is a dummy function used to handle the call of FormatParameters for the last argument, in which case @p args
 * is empty meaning no more arguments to process.
 *
 * The compiler should optimize this function out, since it is inline and empty, but it needs to be here :)
 *
 * @param[in,out] Not used
 */
template <int ArgumentIndex>
void FormatParameters(std::vector<FormatFragment>&) {}

template <int ArgumentIndex, typename T, typename... Args>
void FormatParameters(std::vector<FormatFragment>& fragments, T&& arg, Args&&... args)
{
    FormatParameter<ArgumentIndex>(fragments, arg);
    FormatParameters<ArgumentIndex + 1>(fragments, args...);
}

void OutputFragments(const std::vector<FormatFragment>& fragments, std::ostream& ostr);

//
// Functions generally used for standard formatting of a format string
//

template <typename... Args>
std::string Format(const char* formatStr, Args&&... args)
{
    std::stringstream ostr;
    std::vector<FormatFragment> fragments;
    ParseFormatStr(formatStr, ostr, fragments);
    if (!fragments.empty()) {
        FormatParameters<0>(fragments, args...);
    }
    OutputFragments(fragments, ostr);
    return ostr.str();
}


template <typename... Args>
std::string Format(const std::string& formatStr, Args&&... args)
{
    return Format(formatStr.c_str(), args...);
}


/**
 * The format function used for the cases where no parameters are parsed along.
 *
 * When no parameters are parsed along the format function still parses the format string, note that environment
 * variables are still replaced.
 *
 * @param[in]  formatStr  The format string to parse.
 *
 * @return Returns the formatted string.
 */
std::string Format(const char* formatStr);


/**
 * The std string version of the format function without parameters.
 *
 * This function calls the const char* version of the function, and is equivalent to:
 * @code{.cpp}
 *     return Format(formatStr.c_str());
 * @endcode
 *
 * @param[in]  formatStr  The format string to parse.
 *
 * @return Returns the formatted string.
 */
std::string Format(const std::string& formatStr);

}
}

#endif  /* UTILS_STR_FORMAT_H_ */
