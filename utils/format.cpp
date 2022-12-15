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

#include "format.h"

#include <algorithm>
#include <bitset>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <limits>
#include <string>
#include <cstdlib>

using namespace utils::str;

namespace {

/**
 * Character used for starting a format parameter.
 */
const char FORMAT_START = '{';

/**
 * Character used to end a format parameter.
 */
const char FORMAT_END = '}';

/**
 * Character used within FORMAT_START and FORMAT_END to start a format specifier block.
 */
const char FORMAT_SPECIFIER = ':';

/**
 * Character used after FORMAT_START to identify that this format parameter is about an environment variable.
 */
const char FORMAT_ENVIRONMENT = '$';

/**
 * Character used after the format parameter type is specified to identify that this parameter should be cast to a
 * different type before being formatted.
 */
const char FORMAT_EXPLICIT_TYPE = '!';

const char FORMAT_SELECTOR_OBJ = '.';
const char FORMAT_SELECTOR_ARRAY_BEGIN = '[';
const char FORMAT_SELECTOR_ARRAY_END = ']';

/**
 * Character used to toggle left alignment.
 */
const char FORMAT_ALIGN_LEFT = '<';

/**
 * Character used to toggle right alignment.
 */
const char FORMAT_ALIGN_RIGHT = '>';

/**
 * Character used to toggle center alignment.
 */
const char FORMAT_ALIGN_CENTER = '^';

/**
 * Character used to toggle internal (sign left, number right) alignment.
 */
const char FORMAT_ALIGN_INTERNAL = '=';

/**
 * Character used to toggle the alternate form.
 */
const char FORMAT_ALTERNATE_TOGGLE = '#';

/**
 * Character used to toggle the sign aware zero form.
 */
const char FORMAT_SIGNAWARE_ZERO_TOGGLE = '0';

/**
 * Character used to toggle the thousands separation option.
 */
const char FORMAT_THOUSANDS_TOGGLE = ',';

/**
 * Character used to toggle the precision option.
 */
const char FORMAT_PRECISION_TOGGLE = '.';

/**
 * Format number as percentage, by multiplying with 100 and postfixing a '%'.
 */
const char FORMAT_PERCENTAGE_MODE = '%';

/**
 * Format the number as binary digits.
 */
const char FORMAT_BINARY_TOGGLE = 'b';

/**
 * Format the number as octals.
 */
const char FORMAT_OCTAL_TOGGLE = 'o';

/**
 * Format the number as a hexadecimal string, using lowercase letters.
 */
const char FORMAT_LOWERCASE_HEX_TOGGLE = 'x';

/**
 * Format the number as a hexadecimal string, using uppercase letters.
 */
const char FORMAT_UPPERCASE_HEX_TOGGLE = 'X';

/**
 * Character used to enable sign for negative numbers (default behaviour).
 */
const char FORMAT_SIGN_NEGATIVES_TOGGLE = '-';

/**
 * Character used to toggle having signs shown always (even for postive numbers).
 */
const char FORMAT_SIGN_ALWAYS_TOGGLE = '+';

/**
 * Character to toggle using signs always, but using a space ' ' for positive numbers.
 */
const char FORMAT_SIGN_POSITIVE_SPACE_TOGGLE = ' ';

/**
 * Format the number by using localized formatting rules.
 */
const char FORMAT_LOCALIZED_NUMBER_TOGGLE = 'n';

/**
 * Format the number as the corresponding unicode character (disabled).
 */
const char FORMAT_UNICODE_CHAR_TOGGLE = 'c';

/**
 * Print the number as a regular number (default).
 */
const char FORMAT_NORMAL_NUMBER_TOGGLE = 'd';

/**
 * Format the number using scientific notation.
 */
const char FORMAT_SCIENTIFIC_TOGGLE = 'e';

/**
 * Format the number using upper case scientific notation.
 */
const char FORMAT_SCIENTIFIC_UC_TOGGLE = 'E';

/**
 * Format the number using fixed precision.
 */
const char FORMAT_FIXED_TOGGLE = 'f';

/**
 * Format the number using upper case fixed precision.
 */
const char FORMAT_FIXED_UC_TOGGLE = 'F';

/**
 * Format the number as general format, that is fixed precision, unless the number is too large in which case it is
 * formatted in scientific noation.
 */
const char FORMAT_GENERAL_DECIM_TOGGLE = 'g';

/**
 * Format the number as upper case general format, that is fixed precision, unless the number is too large in which
 * case it is formatted in upper casescientific noation.
 */
const char FORMAT_GENERAL_DECIM_UC_TOGGLE = 'G';

/**
 * Integer value used as index for text fragments.
 */
const int FORMAT_TEXT_INDEX = -1;

/**
 * Integer value used as index for environment fragments.
 */
const int FORMAT_ENVIRONMENT_INDEX = -2;

/**
 * Integer value that identifies that precision has not been specified.
 */
const int PRECISION_NOT_SET = -1;

/**
 * The maximum precision for doubles (unless overwritten manually).
 */
const std::streamsize DOUBLE_MAX_DEFAULT_PRECISION = 16;

/**
 * The minimum precision for double (that is always show one decimal.
 */
const std::streamsize DOUBLE_MIN_DEFAULT_PRECISION = 1;


class AlwaysCommaAsThousandsSep
    : public std::numpunct<char>
{
public:
    /**
     * Make sure we use , as thousands separator.
     *
     * @return Returns , since this is the character we wish to use for thousands separation.
     */
    char do_thousands_sep() const
    {
        return ',';
    }

    /**
     * Make sure we use . as decimal separator.
     *
     * @return Returns . since this is the character we wish to use for decimal separation.
     */
    char do_decimal_point() const
    {
        return '.';
    }

    /**
     * Specify that we wish the thousands separator to be placed every three digits.
     *
     * @return Returns a string where the first character is character with binary value 3, this indicate the group
     *         size. Should be set already, but we want to be absolutely certain no matter the locale.
     */
    std::string do_grouping() const
    {
        return "\3";
    }
};


/**
 * Code inspired by Stack Overflow: http://stackoverflow.com/a/18652393/111143
 */
struct FormatDynamicDecimal
{
    static constexpr const long double precision = std::sqrt(std::numeric_limits<double>::epsilon());
    const long double value;
    const std::streamsize scientificLimit = 5;
    const std::streamsize scientificCeil;
    const std::streamsize maxPrecision;
    const std::streamsize minPrecision;

    FormatDynamicDecimal(long double value, std::streamsize minPrecision, std::streamsize maxPrecision, std::streamsize scientificCeil)
        : value(value), minPrecision(minPrecision), maxPrecision(maxPrecision), scientificCeil(scientificCeil)
    {
    }

    friend std::ostream& operator << (std::ostream& stream, const FormatDynamicDecimal& rhs)
    {
        // We use n to calculate and store the precision to use.
        std::streamsize n = 0;
        // The calculated log10 of the value in an int version.
        unsigned intLogVersion = 0;
        // The number integer digits in the value.
        unsigned intDigits = 0;
        // Whether or not the value is above the scientific ceil value, this is whether the absolute value is so large
        // that it must be written using scientific notation.
        bool aboveScientificCeil = false;
        // The absolute integer version of the number, we use the abs value to avoid having to deal with the sign
        // character.
        uint64_t intVersion = static_cast<uint64_t>(std::abs(rhs.value));
        // By default we use fixed notation and control the number of digits to write, based on our calculations.
        std::ios_base::fmtflags newFlags = std::ios_base::fixed;

        // If the number is not 0, then we might need to use scientific notation because the number is too large.
        // Besides calculating the log10 of the value only makes sense for values larger than 0.
        if (intVersion > 0) {
            intLogVersion = static_cast<unsigned>(std::log10(std::abs(rhs.value)));
            // The number integer digits is the log10 integer value plus 1 since the log10 is the exponent needed in a
            // y = 10 ^ exp to calculate the value y.
            intDigits = intLogVersion + 1;
            // We only see the value as above scientific ceil if the scientific ceil is larger than 0.
            aboveScientificCeil = (intDigits > rhs.scientificCeil) && (rhs.scientificCeil > 0);
        }
        // If the number is above the scientific ceil value we calculate the number of trailing zeroes, these are the
        // digits we can safely avoid writing and thus shouldn't be included in the calculated precision. Note: if we
        // are above the scientific ceil, we will drop the decimal values, since they are considered insignificant.
        // The number 1000 would have 3 trailing zeroes and the number 10100 would have 2.
        if (aboveScientificCeil) {
            std::streamsize trailingZeroes = 0;
            if (intVersion > 0) {
                for (unsigned factor = 10; (intVersion % factor) == 0; factor *= 10) {
                    ++trailingZeroes;
                }
            }
            // There is no need to consider the tailing zeroes in the precision we use, so we subtract them from it.
            n = intLogVersion - trailingZeroes;
        }
        else if (intDigits < rhs.maxPrecision) {
            // Leading zeroes is the number of zeroes after the decimal sign before any other significant digit,
            // leading zeroes is always 0 if the integer value is above 0.
            int leadingZeroes = 0;
            // Remember leading zeroes only makes sense when the integer version of the number is 0. This variable is
            // used to store when we are processing a digit that is a leading zero or not.
            bool inLeadingZeroes = intVersion == 0;
            // This is the value that is left when we subtract the truncated value, this is used to check if we have
            // processed all digits. We check the value against our precision which is a small enough value that it
            // should be capable of detecting when we have reached the last digit.
            long double f = std::abs(rhs.value - std::trunc(rhs.value));
            while (precision < f) {
                // Multiply by 10 so we can extract the actual digit.
                f *= 10;
                if (inLeadingZeroes && std::trunc(f) < precision) {
                    ++leadingZeroes;
                }
                else {
                    inLeadingZeroes = false;
                }
                // Remove this digit so we can extract the next.
                f -= std::trunc(f);
                // Since this as a significant digit (or a leading zero) we should increase the required precision.
                ++n;
            }
            // The precision we have found should not be any less than the minimum precision, the format string may
            // have specified that we should display at least a number of digits, and so we must respect that.
            n = std::max(n, rhs.minPrecision);

            // If the number of digits we have found is above the scientific limit and we have leading zeroes we should
            // use scientific notation, and discard the leading zeroes, we know n will be larger than the number of
            // leading zeroes so we can safely subtract them. We subtract one so we have atleast the one digit before
            // the decimal point.
            if (n >= rhs.scientificLimit && leadingZeroes > 0) {
                newFlags = std::ios_base::scientific;
                n -= leadingZeroes + 1;
            }
        }

        // We do not want to use more than at most the maximum specified precision, so adjust down to it if needed.
        n = std::min(n, rhs.maxPrecision);
        if (aboveScientificCeil) {
            // Since we are above the scientific ceil we should use scientific notation.
            newFlags = std::ios_base::scientific;
            // If we have reached the max precision we need to remove one, since scientific notation takes up one.
            if (n == rhs.maxPrecision) {
                --n;
            }
        }
        else if (intDigits + n > rhs.maxPrecision) {
            // The number of integer digits plus the precision we strive to use, is more than the maximum precision,
            // so there is not enough digits to show the integer with the digits. We need to cut off a few digits,
            // since we obviously value the integer part more than the decimal part, we will attempt to keep that part
            // and cut decimals off as required.
            if (intDigits < n) {
                n -= intDigits;
            }
            else {
                n = 0;
            }
        }
        // Set the precision and flags, storing the old ones, write the value and be a good boy and restore the
        // originals.
        n = stream.precision(n);
        std::ios_base::fmtflags oldFlags = stream.setf(newFlags, std::ios_base::floatfield);
        stream << rhs.value;
        stream.flags(oldFlags);
        stream.precision(n);

        return stream;
    }
};


/**
 * Initialise a format fragment struct, setting the index, and explicity conversion.
 *
 * The index defaults to FORMAT_TEXT_INDEX which is the index used by text fragments, the explicit type conversion
 * member is set to character 0.  If the FORMAT_DISABLE_THROW_OUT_OF_RANGE macro is not defined handled is set to true
 * for indexes having a value of less than 0, since these fragments (text and environnent variables) do not refer to a
 * format argument.
 *
 * @param[out] fragment  The fragment to initialise.
 * @param[in]  index     The index to use for the fragment, this defaults to FORMAT_TEXT_INDEX.
 */
void InitializeFormatFragment(FormatFragment& fragment, int index = FORMAT_TEXT_INDEX) noexcept
{
    fragment.index = index;
    fragment.explicitConversion = '\0';
#ifndef FORMAT_DISABLE_THROW_OUT_OF_RANGE
    // By default format parameters are not handled.
    fragment.handled = index < 0;
#endif  // FORMAT_DISABLE_THROW_OUT_OF_RANGE
}


/**
 * Initialises a basic format specifier.
 *
 * Fills the provided format specifier struct, setting everything to the default value, precision is set to the not set
 * value.
 *
 * @param[out] specifier The basic format specifier to fill.
 */
void InitializeFormatSpecifier(BasicFormatSpecifiers& specifier) noexcept
{
    specifier.precision = PRECISION_NOT_SET;
    specifier.width = 0;
    specifier.align = '\0';
    specifier.fill = '\0';
    specifier.sign = FORMAT_SIGN_NEGATIVES_TOGGLE;
    specifier.type = '\0';
    specifier.alternateForm = false;
    specifier.thousandSeparator = false;
}


/**
 * Parses a string, extracting all text from the string, including any escaped format start or end character, it may
 * encounter.
 *
 * The function starts at position @p pos in @p formatParameter, reading one character at a time, it continues to
 * parse the string until it encounters either a character with the value @c FORMAT_START, that is not escaped, or the
 * null-terminator. The value of @p pos will be the index of the last character that was examined by this function, if
 * the function stops parsing due to an unescaped @c FORMAT_START, @p pos will be the index of this character.
 *
 * If the function encounters an escaped @c FORMAT_START or escaped @c FORMAT_END, the escaped value will be written
 * to the output stream @p ostr, that is the value that was escaped (excluding the escape character).
 *
 * @exception IllegalFormatStringException  Throws an exception if an unescaped @c FORMAT_END is encountered.
 *
 * @param[in]     formatParameter  The format parameter text to read.
 * @param[out]    ostr  The output stream, to write the result string to.
 * @param[in,out] pos  The position to read from, and the position at which reading stopped.
 */
void GetPlainTextFragment(const char* formatParameter, std::ostream& ostr, int& pos)
{
    char expect = '\0';
    while (formatParameter[pos]) {
        char c = formatParameter[pos];
        if (expect && expect != c) {
            throw IllegalFormatStringException(formatParameter, pos,
                                               "Expected a different character, is this supposed to be escaped?");
        }

        switch (c) {
            case FORMAT_START:
                // Look ahead and check if this is an escaped opening bracket
                if (formatParameter[pos + 1] == FORMAT_START) {
                    ostr << c;
                    ++pos;
                }
                else {
                    // This was not an escaped opening bracket, so we should stop here, and let some one else handle
                    // this from here.
                    return;
                }
                break;

            case FORMAT_END:
                if (expect == FORMAT_END) {
                    ostr << c;
                    expect = '\0';
                }
                else if (expect == '\0') {
                    expect = FORMAT_END;
                }
                break;

            default:
                ostr << c;
        }

        ++pos;
    }
}


/**
 * Finds the first non-white space character in a string starting from a specific position.
 *
 * Examines the string @p formatParameter one character at a time, finding the first character that is either not a
 * white space character or is the null terminator.  The search starts at the position specified by @p pos, and ends
 * at the character that is not a whitespace, @p pos is updated to the location of this character.
 *
 * White space characters are one of the following: space, tab, newline, or carriage return.
 *
 * @param[in] formatParameter  The string to examine.
 * @param[in,out] pos  The position to begin searching from, when the function exits this parameter holds the position
 *                of the last character it examined (never a white space).
 */
void SkipWhitespace(const char* formatParameter, int& pos) noexcept
{
    char c = formatParameter[pos];
    while (c) {
        switch (c) {
            case ' ':   // Fall through
            case '\t':  // Fall through
            case '\n':  // Fall through
            case '\r':
                break;

            default:
                return;
        }
        ++pos;
        c = formatParameter[pos];
    }
}


/**
 * Parses a string reading an integer number from it.
 *
 * Parses the string @p formatParameter starting from position @p pos, and continuing until the first character that
 * is not part of an integer. If the first character encountered is not part of an integer (ie. being a letter) the
 * function exits not updating @p pos, returning @p defaultValue.
 *
 * The read integer is always assumed to be in base 10, any regular character identifying it as either hex (normally
 * x) or octal (normally o), or any other, are identified as letters, and will cause the integer reading to stop at
 * the position of the character.
 *
 * @exception IllegalFormatStringException An exception is thrown if a signed integer is found, and @p allowNegative
 *            is false.
 * @exception IllegalFormatStringException An exception is thrown if a sign character is found anywhere but the first
 *            character.
 * @exception IllegalFormatStringException An exception is thrown if the integer is too large to be represented by a
 *            regular @c int data type, the error position will be the digit in the string causing the integer to
 *            overflow.
 * @exception IllegalFormatStringException An exception is thrown if the found integer is written as -0, if this
 *            occurs pos is set to the first character after the number as normal.
 *
 * @param[in] formatParameter  The string to parse and read the integer from.
 * @param[in,out] pos  The position to start parsing from, and when the function exits, the position of the character
 *            directly following the integer number, that is the first character not part of the integer\. If the
 *            function exits due to an exception, the value will be that of the character causing the exception to be
 *            thrown.
 * @param[in] allowNegative  Indicates whether to allow negative values or not, if this is set to false negative
 *            integers are not allowed, and finding any such will result in an exception.
 * @param[in] defaultValue  The value to return if no integer value was found.
 *
 * @return Returns the found integer value, or @p defaultValue if no integer value was found.
 */
int ParseIntegerNumber(const char* formatParameter, int& pos, bool allowNegative, int defaultValue)
{
    // The integer we have parsed, this will be constructed on the fly.
    int parsedInteger = 0;
    // This is 1 for positive number and -1 for negatives.
    int sign = 1;
    // Remember whether we have read past the first character to detect falsely positioned sign characters.
    bool firstChar = true;
    // Have we actually read any integer values (0 is an acceptable integer), or should we revert to use the default
    // value.
    bool foundAny = false;

    char c = formatParameter[pos];
    while (c) {
        if (c == '-') {
            foundAny = true;
            if (allowNegative && firstChar) {
                sign = -1;
            }
            else {
                throw IllegalFormatStringException(formatParameter, pos, "A sign character is not allowed at this position");
            }
        }
        else if (c >= '0' && c <= '9') {
            foundAny = true;
            parsedInteger = parsedInteger * 10 + (c - '0');
            // An amount so large it overflows the integer
            if (parsedInteger < 0) {
                throw IllegalFormatStringException(formatParameter, pos, "Integer value overflows, use a smaller number");
            }
        }
        else {
            break;
        }

        firstChar = false;
        ++pos;
        c = formatParameter[pos];
    }

    if (!foundAny) {
        parsedInteger = defaultValue;
    }

    if (parsedInteger == 0 && sign < 0) {
        throw IllegalFormatStringException(formatParameter, pos, "-0 is not a valid integer");
    }

    return parsedInteger * sign;
}


/**
 * Reads alignment specification from the format parameter, storing it in a basic format specifier struct.
 *
 * Attempts to read an alignment specifier and fill character (if set) from the given string.  If no fill or align
 * information is found, @p pos is unchanged.  If only alignment information is found, but no fill character @p pos is
 * increased by 1, if both alignment and fill character is found @p pos is increased by 2.
 *
 * @param[in] formatParameter  The string to read alignment specifier from.
 * @param[out] fragment  The format specifier to store the alignment and fill information in.
 * @param[in,out] pos  The position to start reading from, and when the function exits, the position of the first
 *                character not part of the alignment or fill specification.
 */
void ReadAlignSpecifier(const char* formatParameter, BasicFormatSpecifiers& fragment, int& pos) noexcept
{
    if (formatParameter[pos] && formatParameter[pos + 1]) {
        // The first guess is that both fill and align is specified
        char fill = formatParameter[pos];
        char align = formatParameter[pos + 1];

        // We can assume that if align is one of the four alignment tokens this was correct
        if (align == FORMAT_ALIGN_LEFT
                || align == FORMAT_ALIGN_RIGHT
                || align == FORMAT_ALIGN_INTERNAL
                || align == FORMAT_ALIGN_CENTER) {
            fragment.align = align;
            fragment.fill = fill;
            pos += 2;
        }
        else if (fill == FORMAT_ALIGN_LEFT
                || fill == FORMAT_ALIGN_RIGHT
                || fill == FORMAT_ALIGN_INTERNAL
                || fill == FORMAT_ALIGN_CENTER) {
            // Align was not the correct align field, perhaps fill was not specified, so align is in fill's position.
            fragment.align = fill;
            fragment.fill = '\0';
            pos += 1;
        }
    }
}


/**
 * Reads a sign specifier, if any, from a string from a given position.
 *
 * Attempts to read a sign character ('+', '-', ' ') from the string @p formatParameter at position @p pos.  If such a
 * character is found it is written to the sign member of @p fragment, and @p pos is increased by 1.
 *
 * @param[in] formatParameter  The string to read the sign character from.
 * @param[out] fragment  The format fragment specifier to write the information in.
 * @param[in,out] pos  The location in the string to find the sign character at, if found this is increased by 1.
 */
void ReadSignSpecifier(const char* formatParameter, BasicFormatSpecifiers& fragment, int& pos) noexcept
{
    char sign = formatParameter[pos];
    // Was a sign character specified?
    if (sign == FORMAT_SIGN_ALWAYS_TOGGLE || sign == FORMAT_SIGN_NEGATIVES_TOGGLE || sign == FORMAT_SIGN_POSITIVE_SPACE_TOGGLE) {
        fragment.sign = sign;
        ++pos;
    }
}


/**
 * Reads an alternate form specifier, if any, form a string from a given position.
 *
 * Attempts to read a alternate form specifier (FORMAT_ALTERNATE_TOGGLE) from the string @p formatParameter at
 * position @p pos.  If it is found at this location, it is written to the @c alternateForm member of @p fragment, and
 * @p pos is increased by 1.
 *
 * @param[in] formatParameter  The string to read the alternate form specifier from.
 * @param[out] fragment  The format fragment to write the information in.
 * @param[in,out] pos  The location in the string to find the alternate form at, if found this is increased by 1.
 */
void ReadAlternateSpecifier(const char* formatParameter, BasicFormatSpecifiers& fragment, int& pos) noexcept
{
    if (formatParameter[pos] == FORMAT_ALTERNATE_TOGGLE) {
        fragment.alternateForm = true;
        ++pos;
    }
}


/**
 * Reads the sign aware zero specifier from a string at given position.
 *
 * Attempts to read the sign aware zero specifier (FORMAT_SIGNAWARE_ZERO_TOGGLE) from the string @p formatParameter,
 * at position @p pos.  If it is found @p pos is increased by 1, and it is stored in the @c fill member of
 * @p fragment.  If the @c align member of @p fragment is null the align specifier is set to '=', to emulate Python
 * behavior.
 *
 * @param[in] formatParameter  The string to read the sign aware zero specifier from.
 * @param[in,out] fragment  The fragment to write the specifier to, and test align specifier from.
 * @param[in,out] pos  The position to read the specifier from, if found this is increased by 1.
 */
void ReadSignAwareZeroSpecifier(const char* formatParameter, BasicFormatSpecifiers& fragment, int& pos) noexcept
{
    if (formatParameter[pos] == FORMAT_SIGNAWARE_ZERO_TOGGLE) {
        fragment.fill = '0';
        if (fragment.align == '\0') {
            fragment.align = FORMAT_ALIGN_INTERNAL;
        }
        ++pos;
    }
}


/**
 * Attempt to read a width specifier from a string.
 *
 * Reads a width specifier from the string @p formatParameter starting from position @p pos, storing it in the member
 * @c width of @p fragment.  If no integer is found at this position 0 is stored as width.  The position @p pos is the
 * position of the first character after the number that is not part of the number.
 *
 * @param[in] formatParameter  The string to scan for the width specifier.
 * @param[out] fragment  The format specifier to write the width to.
 * @param[in,out] pos  The position in the string to search from, and when complete the position of the first
 *                character not part of the number.
 */
void ReadWidthSpecifier(const char* formatParameter, BasicFormatSpecifiers& fragment, int& pos)
{
    fragment.width = ParseIntegerNumber(formatParameter, pos, false, 0);
}


/**
 * Attempts to read the thousands specifier option from the format string.
 *
 * Checks the charcter at position @p pos in the format string @p formatParameter, if this character is
 * @c FORMAT_THOUSANDS_TOGGLE the @c thousandSeparator member of @p fragment is set to true, and @p pos is increased
 * by 1.
 *
 * @param[in] formatParameter  The format string to look in.
 * @param[out] fragment  The format specifier fragment to write the result to.
 * @param[in,out] pos  The position to look in, and update if found.
 */
void ReadThousandSepSpecifier(const char* formatParameter, BasicFormatSpecifiers& fragment, int& pos) noexcept
{
    if (formatParameter[pos] == FORMAT_THOUSANDS_TOGGLE) {
        fragment.thousandSeparator = true;
        ++pos;
    }
}


/**
 * Reads precision toggle from the format string at the given position.
 *
 * Attempts to read the precision, this is only attempted if the character in the format string at the position
 * specified by @p pos is FORMAT_PRECISION_TOGGLE. If this is the case @p pos is increased by 1, and the characters
 * following it, is read as a number, increasing @p pos as required.
 *
 * @param[in] formatParameter  The format string to parse.
 * @param[out] fragment  The format fragment to store the precision in, if found, otherwise it is left untouched.
 * @param[in,out] pos  The position to read the precision from, if found it is increased.
 */
void ReadPrecisionSpecifier(const char* formatParameter, BasicFormatSpecifiers& fragment, int& pos)
{
    if (formatParameter[pos] == FORMAT_PRECISION_TOGGLE) {
        ++pos;
        fragment.precision = ParseIntegerNumber(formatParameter, pos, false, -1);
    }
}


/**
 * Reads the type specifier from the format string at the given position.
 *
 * Attempts to read the type specifier from the format string, if this is one of the allowed type specifiers, we copy
 * it to the format specifier @p fragment, and increase the position @p pos.
 *
 * @param[in] formatParameter  The format string to read the type specifier from.
 * @param[out] fragment  The format specifier to store the found type in if found.
 * @param[in,out] pos  The position at which to read the parameter, increased if found.
 */
void ReadTypeSpecifier(const char* formatParameter, BasicFormatSpecifiers& fragment, int& pos) noexcept
{
    char type = formatParameter[pos];
    if (type == FORMAT_BINARY_TOGGLE
            || type == FORMAT_UNICODE_CHAR_TOGGLE
            || type == FORMAT_NORMAL_NUMBER_TOGGLE
            || type == FORMAT_SCIENTIFIC_TOGGLE
            || type == FORMAT_SCIENTIFIC_UC_TOGGLE
            || type == FORMAT_FIXED_TOGGLE
            || type == FORMAT_FIXED_UC_TOGGLE
            || type == FORMAT_GENERAL_DECIM_TOGGLE
            || type == FORMAT_GENERAL_DECIM_UC_TOGGLE
            || type == FORMAT_LOCALIZED_NUMBER_TOGGLE
            || type == FORMAT_OCTAL_TOGGLE
            || type == FORMAT_LOWERCASE_HEX_TOGGLE
            || type == FORMAT_UPPERCASE_HEX_TOGGLE
            || type == FORMAT_PERCENTAGE_MODE) {
        fragment.type = type;
        ++pos;
    }
}


void ReadIdentifier(const char* formatParameter, std::ostream& ostr, int& pos) noexcept
{
    char c = formatParameter[pos];

    while (c) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
            ostr << c;
        }
        else {
            return;
        }

        ++pos;
        c = formatParameter[pos];
    }
}


/**
 * Reads an environment variable name, when this function is called the current position in the input string must
 * always be the FORMAT_ENVIRONMENT character.
 *
 * When formatting strings it is possible to insert the contents of environment variables directly into the text,
 * using environment variable identifiers, that is format parameter references starting with the FORMAT_ENVIRONMENT
 * character.
 *
 * Environment names can is in this scope limited to characters: a-z, A-Z, 0-0 and _. Since there are no real standard
 * definition of how environment names may be named, this is the convention used here.  The name is read character by
 * character until a character is found that is not one of the ones mentioned previously.  When this function returns
 * @p pos is updated to be the index of the first character not part of the environment name.
 *
 * @param[in] formatParameter  The format string to parse, starting from position @p pos.
 * @param[out] ostr  The output stream to write the found environment name to.
 * @param[in,out] pos  The position to start parsing from, and when completed the position of the first character not
 *                part of the environment name.
 */
void ReadEnvironmentVariableName(const char* formatParameter, std::ostream& ostr, int& pos) noexcept
{
    assert(formatParameter[pos] == FORMAT_ENVIRONMENT);
    ++pos;
    ReadIdentifier(formatParameter, ostr, pos);
}


/**
 * Reads the format specifier text, that is the text following FORMAT_SPECIFIER character.
 *
 * The format specifier starts after the FORMAT_SPECIFIER character and continues until the first non-escaped
 * FORMAT_END character.  Any escaped FORMAT_START, and FORMAT_END characters encountered are escaped in the resulting
 * string.  If an unescaped FORMAT_START is encountered an exception is thrown.
 *
 * This function starts reading from position @p pos and when finished @p pos is updated to the position of the
 * FORMAT_END character. If an exception is thrown the @p pos parameter will be the position of the character causing
 * the exception.
 *
 * @exception IllegalFormatStringException If an unescaped FORMAT_START character is encountered an exception of this
 *            type is thrown.
 *
 * @param[in] formatParameter  The format string to read specifier from.
 * @param[in,out] pos  The position to start reading from, and when complete the position of the FORMAT_END character.
 * @param[out] ostr  The output stream to write the format specifier to.
 */
void ReadFormatSpecifier(const char* formatParameter, int& pos, std::ostream& ostr)
{
    char c = formatParameter[pos];
    while (c) {
        if (c == FORMAT_END) {
            if (formatParameter[pos + 1] == FORMAT_END) {
                ostr << c;
                ++pos;
            }
            else {
                return;
            }
        }
        else if (c == FORMAT_START) {
            if (formatParameter[pos + 1] == FORMAT_START) {
                ostr << c;
                ++pos;
            }
            else {
                throw IllegalFormatStringException(formatParameter, pos,
                                                   "Expected a different character, is this supposed to be escaped?");
            }
        }
        else {
            ostr << c;
        }

        ++pos;
        c = formatParameter[pos];
    }
}


/**
 * Reads explicit type conversion characters from the format string.
 *
 * The explicit type conversion character: FORMAT_EXPLICIT_TYPE starts a type conversion specifier, in this a type can
 * be specified that the input parameter should be converted to before being processed.  The format method excepts one
 * of four possible types: s, r, i, and d.
 *
 * If the character at position @p pos in the input string @p formatParameter is not the character:
 * FORMAT_EXPLICIT_TYPE, then nothing is done, and the function simply exits. If the found conversion character is not
 * one of the accepted four, an exception is thrown.
 *
 * @exception IllegalFormatStringException An exception is thrown if the format type conversion character is not one
 *            of: s, r, i, or d.
 *
 * @param[in] formatParameter  The format string to read the conversion character from.
 * @param[out] fragment  The format fragment to write the conversion type to.
 * @param[in,out] pos  The position of the FORMAT_EXPLICIT_TYPE character to start reading from, when the function
 *                exits @p pos is updated to the index of the character after the type conversion character, or if
 *                nothing is read it is not changed at all.
 */
void ReadExplicitTypeConversion(const char* formatParameter, FormatFragment& fragment, int& pos)
{
    if (formatParameter[pos] == FORMAT_EXPLICIT_TYPE) {
        ++pos;
        char type = formatParameter[pos];
        if (strchr("srid", type) == nullptr) {
            throw IllegalFormatStringException(formatParameter, pos,
                                               "Unknown format conversion specifier, expected one of: s, r, i, and d");
        }
        fragment.explicitConversion = type;
        ++pos;
    }
}


void ReadSelector(const char* formatParameter, FormatFragment& fragment, int& pos)
{
    char selectorType = formatParameter[pos];
    if (selectorType == FORMAT_SELECTOR_OBJ || selectorType == FORMAT_SELECTOR_ARRAY_BEGIN) {
        std::stringstream buffer;
        ++pos;
        ReadIdentifier(formatParameter, buffer, pos);
        char endSelector = formatParameter[pos];
        if (selectorType == FORMAT_SELECTOR_ARRAY_BEGIN && endSelector != FORMAT_SELECTOR_ARRAY_END) {
            throw IllegalFormatStringException(formatParameter, pos, "Illegal selector syntax");
        }
        if (endSelector == FORMAT_SELECTOR_ARRAY_END) {
            ++pos;
        }
        fragment.selectors.push(buffer.str());
    }
}


void ReadSelectors(const char* formatParameter, FormatFragment& fragment, int& pos)
{
    char selectorType = formatParameter[pos];
    while (selectorType == FORMAT_SELECTOR_OBJ || selectorType == FORMAT_SELECTOR_ARRAY_BEGIN) {
        ReadSelector(formatParameter, fragment, pos);
        selectorType = formatParameter[pos];
    }
}


/**
 * Translates a format fragment if it contains an environment variable reference.
 *
 * If the index is FORMAT_ENVIRONMENT_INDEX the environment with the name from text property of @p fragment is
 * retrieved and stored in the text property of the fragment parameter.
 *
 * @param[in,out] fragment The format fragment to check, if the index is FORMAT_ENVIRONMENT_INDEX the text property
 *                filled with the value of the environment value.
 */
void TranslateEnvironmentFragment(FormatFragment& fragment)
{
    if (fragment.index == FORMAT_ENVIRONMENT_INDEX) {
        // Retrieve the environment variable as a pointer, we do not assign this directly to the string envValue since
        // parsing a null pointer to the string constructor will result in an exception, instead we convert null
        // pointers to empty strings.
        const char* envValuePtr = std::getenv(fragment.text.c_str());
        std::string envValue = (envValuePtr != nullptr ? envValuePtr : "");
        std::stringstream buffer;
        // Environment variables are handled directly during the parsing step, this is done to prevent handling these
        // parameter multiple times.  Environment variables are always handled as strings, but it is obviously still
        // possible to explicitly handle them as numbers, using the FORMAT_EXPLICIT_TYPE possibility.
        if (!ConvertAndFormatType(envValue, fragment, buffer)) {
            FormatType(envValue, fragment.formatSpecifier, buffer);
        }
        // Set the text to use
        fragment.text = buffer.str();
    }
}


/**
 * Parses the format string extracting the format specifier, if the character at position @p pos is FORMAT_SPECIFIER.
 *
 * If the current character, that is the character at position @p pos is FORMAT_SPECIFIER, then the format specifier
 * text is extracted, and stored into the @c formatSpecifier property of @p fragment.  The position @p pos is updated
 * to the index of the first character that is not part of the format specifier, when the function exits.
 *
 * @param[in] formatParameter  The format string to parse.
 * @param[out] fragment  The format fragment to store the format specifier in.
 * @param[in,out] pos  The position within the format parameter to read the format specifier from, this must be the
 *                character identifying that this is a format specifier, when the function exits it is the position of
 *                the FORMAT_END character.
 */
void ParseFormatSpecifier(const char* formatParameter, FormatFragment& fragment, int& pos)
{
    // Format specifiers are not mandatory, so only continue if this is a format specifier.
    if (formatParameter[pos] == FORMAT_SPECIFIER) {
        ++pos;
        std::stringstream buffer;
        ReadFormatSpecifier(formatParameter, pos, buffer);
        fragment.formatSpecifier = buffer.str();
    }
}


/**
 * Parses a format parameter starting from a format parameter start character, and includes any text fragment that
 * follows it.
 *
 * This function always assumes that the current position is a format parameter start character, the format parameter
 * is then parsed and stored in a @c FormatFragment struct, which is appended to the @p fragments vector.  If no index
 * is found on the format parameter, the index provided by the @p nextParameterIndex parameter is used.  The
 * @p nextParameterIndex parameter is increased by 1, if the parameter is not an environment variable.
 *
 * After having parsed a format fragment, any text part following it, is parsed and added as a format fragment of its
 * own. That has the effect, that after this function has been called, the position is either that of a new format
 * parameter, or then null terminator of the string.  This is a guarantee the function provides, simplifying using
 * parsing using it.
 *
 * @param[in] formatParameter  The format parameter to parse.
 * @param[out] fragments  A vector of fragments to append the extracted format fragment to.
 * @param[in,out] pos  The position to start parsing from, and when complete, the position of the next format
 *                parameter, or null terminator of the string.
 * @param[in,out] nextParameterIndex  The next parameter index to use if no index is provided, this will be updated if
 *                the format fragment is not an environment.
 */
void ParseFormatParameter(const char* formatParameter, std::vector<FormatFragment>& fragments, int& pos,
        int& nextParameterIndex)
{
    assert(formatParameter[pos] == FORMAT_START);
    FormatFragment fragment;
    int parameterIndex;

    // Skip past the opening bracket
    ++pos;
    SkipWhitespace(formatParameter, pos);

    // Is this an environment format?
    if (formatParameter[pos] == FORMAT_ENVIRONMENT) {
        parameterIndex = FORMAT_ENVIRONMENT_INDEX;
        std::stringstream buffer;
        ReadEnvironmentVariableName(formatParameter, buffer, pos);
        fragment.text = buffer.str();
    }
    else {
        // This is a regular
        parameterIndex = ParseIntegerNumber(formatParameter, pos, false, nextParameterIndex);
        nextParameterIndex = parameterIndex + 1;
    }

    InitializeFormatFragment(fragment, parameterIndex);

    // Read selectors
    ReadSelectors(formatParameter, fragment, pos);

    // Do we have any explicit type conversions
    ReadExplicitTypeConversion(formatParameter, fragment, pos);

    // Fill out the format fragment
    ParseFormatSpecifier(formatParameter, fragment, pos);

    // Handle environment variables directly
    TranslateEnvironmentFragment(fragment);
    fragments.push_back(fragment);

    // Skip to closing bracket
    SkipWhitespace(formatParameter, pos);
    if (formatParameter[pos] != FORMAT_END) {
        throw IllegalFormatStringException(formatParameter, pos, "Expected format closing bracket '}'");
    }
    ++pos;

    if (formatParameter[pos]) {
        std::stringstream buffer;
        GetPlainTextFragment(formatParameter, buffer, pos);
        if (buffer.tellp() > 0) {
            FormatFragment textFragment;
            InitializeFormatFragment(textFragment);
            textFragment.text = buffer.str();
            fragments.push_back(textFragment);
        }
    }
}


/**
 * This function prepares the output stream for outputting the integer format from the format specifiers in
 * @p specifiers.
 *
 * When calling this function make sure to check the value of @p useValue, if this is false then do not print the
 * value to the stream as this has already been done.
 *
 * @param[in]     specifiers  The format specifiers, this is the parsed format options.
 * @param[in,out] ostr  The stream to configure, and to use for output.
 * @param[in,out] value  The value to format and output.
 * @param[out]    useValue  A boolean value of true or false, specifying whether to write the value or not, if this is
 *                set to false, the value should not be written by the caller, because it is already written.
 * @param[out]    useDynamic  Indicates whether to use use dynamic notation (not fixed) or not.
 * @param[out]    minPrecision  The minimum precision to use when representing this decimal number.
 * @param[in,out] maxPrecision  The maximum precision to use when representing this decimal number.
 * @param[out]    scientificCeil  The upper limit for switching to scientific notation.
 */
void PreprocessStreamForDecimal(const BasicFormatSpecifiers& specifiers, std::ostream& ostr, long double& value,
        bool& useValue, bool& useDynamic, std::streamsize& minPrecision, std::streamsize& maxPrecision,
        std::streamsize& scientificCeil)
{
    // By default use the value.
    useValue = true;

    // By default use dynamic notation
    useDynamic = true;

    // By default do not use the scientific ceil, that is upper limit for switching to scientific notation
    scientificCeil = 0;

    // The default minimum precision to use
    minPrecision = 1;

    // Set the minimum width of the output
    ostr << std::setw(specifiers.width);

    // Set the fill character if it needs to be set
    if (specifiers.fill) {
        ostr << std::setfill(specifiers.fill);
    }

    // Set the precision
    if (specifiers.precision >= 0) {
        ostr << std::setprecision(specifiers.precision);
    }

    // If thousands separator is set imbue a locale, since the PEP-3101 dictates that the thousands separator should
    // be a ',' we use a custom numpunct class reporting the thousands separator as a comma.
    if (specifiers.thousandSeparator) {
        std::locale loc(ostr.getloc(), new AlwaysCommaAsThousandsSep);
        ostr.imbue(loc);
    }

    switch (specifiers.sign) {
        // Nothing really needs to be done for this, this is default behavior, show a sign character for negatives
        // only.
        case FORMAT_SIGN_NEGATIVES_TOGGLE:
            break;

        // When setting the sign character to +, numbers will always have a sign of either + or -. Negative numbers
        // will have a - sign, and 0 and positive numbers will have a +.
        case FORMAT_SIGN_ALWAYS_TOGGLE:
            ostr << std::showpos;
            break;

        // Like with +, this will always show a sign character, however the sign character for 0 or positive numbers
        // will be a space. This sets the width of the stream to 0, as it requires some post processing, it is simply
        // not supported directly by the underlying stream.
        case FORMAT_SIGN_POSITIVE_SPACE_TOGGLE:
            ostr << std::setw(0);
            break;

        // Ignore unknown sign characters.
        default:
            break;
    }

    switch (specifiers.align) {
        // Will align the value to the left:
        // [-xxxx    ]
        case FORMAT_ALIGN_LEFT:
            ostr << std::left;
            break;

        // Will align the value to the right, and the sign to the left:
        // [-    xxxx]
        case FORMAT_ALIGN_INTERNAL:
            ostr << std::internal;
            break;

        // Will center the value and sign:
        // [  -xxxx  ]
        case FORMAT_ALIGN_CENTER:
            // This needs to be processed afterwards, for now make sure we only have the raw data
            ostr << std::setw(0);
            break;

        // Will align the value to the right:
        // [    -xxxx]
        case FORMAT_ALIGN_RIGHT:
        default:
            ostr << std::right;
    }

    switch (specifiers.type) {
        case FORMAT_SCIENTIFIC_UC_TOGGLE:
            ostr << std::uppercase;
            // no break
        case FORMAT_SCIENTIFIC_TOGGLE:
            if (specifiers.precision == PRECISION_NOT_SET) {
                useDynamic = false;
                ostr << std::setprecision(6);
            }
            ostr << std::scientific;
            break;

        case FORMAT_GENERAL_DECIM_UC_TOGGLE:
            ostr << std::uppercase;
            // no break
        case FORMAT_GENERAL_DECIM_TOGGLE:
            {
                minPrecision = 0;
                maxPrecision = 6;
                scientificCeil = 6;
            }
            break;

        case FORMAT_LOCALIZED_NUMBER_TOGGLE:
            {
                minPrecision = 0;
                maxPrecision = 6;
                scientificCeil = 6;
                ostr.imbue(std::locale(""));
            }
            break;

        case FORMAT_PERCENTAGE_MODE:
            value *= 100.0;
            // no break
        case FORMAT_FIXED_UC_TOGGLE:
            ostr << std::uppercase;
            // no break
        case FORMAT_FIXED_TOGGLE:
            if (specifiers.precision == PRECISION_NOT_SET) {
                useDynamic = false;
                ostr << std::setprecision(6);
            }
            ostr << std::fixed;
            break;

        // Ordinary decimal number
        case FORMAT_NORMAL_NUMBER_TOGGLE:
        default:
            break;
    }
}


/**
 * Post processing written doubles to the output stream.
 *
 * Prints the written string in the following format to the ostr stream:
 *
 * [left padding][sign][type][fill][written string][right padding]
 *
 * @param[in] writtenString  The actual string that should be written to the output string if no post processing is
 *            needed.
 * @param[in] specifiers  The format specifiers used for this
 * @param[out] ostr  The output string to write the finalized string to.
 * @param[in] value  The value that should be formatted and output.
 */
void PostprocessStreamForDecimal(std::string&& writtenString, const BasicFormatSpecifiers& specifiers,
        std::ostream& ostr, const long double& value)
{
    int contentWidth = static_cast<int>(writtenString.size());
    bool addPadding = (specifiers.sign == FORMAT_SIGN_POSITIVE_SPACE_TOGGLE || specifiers.align == FORMAT_ALIGN_CENTER);
    char fillCharToUse = specifiers.fill ? specifiers.fill : ' ';

    // If the sign is a space, we should make room for it in the calculations, it has not been added to the
    // writtenString yet.
    if (specifiers.sign == FORMAT_SIGN_POSITIVE_SPACE_TOGGLE && value >= 0) {
        ++contentWidth;
    }
    else if (specifiers.sign == FORMAT_SIGN_POSITIVE_SPACE_TOGGLE && value < 0 && specifiers.align == FORMAT_ALIGN_INTERNAL) {
        // Remove sign, we do not need to add to contentWidth here since the sign was included.
        writtenString.erase(0, 1);
    }

    // If the number should be prefixed with the base (0b, 0o, 0x, 0X) make room for it.
    if (specifiers.alternateForm && strchr("boxX", specifiers.type) != nullptr) {
        contentWidth += 2;
    }

    // Calculate the paddings: [left][sign][center][value][right]
    int paddingLeft = 0;
    int paddingCenter = 0;
    int paddingRight = 0;
    if (addPadding) {
        switch (specifiers.align) {
            case FORMAT_ALIGN_LEFT:
                paddingRight = specifiers.width - contentWidth;
                break;

            case FORMAT_ALIGN_CENTER:
                paddingLeft = (specifiers.width - contentWidth) / 2;
                paddingRight = specifiers.width - paddingLeft - contentWidth;
                break;

            case FORMAT_ALIGN_INTERNAL:
                paddingCenter = specifiers.width - contentWidth;
                break;

            case FORMAT_ALIGN_RIGHT:
            default:
                paddingLeft = specifiers.width - contentWidth;
                break;
        }
    }

    // Add the left padding if any.
    if (addPadding && paddingLeft > 0) {
        ostr << std::setw(paddingLeft)
             << std::setfill(fillCharToUse)
             << fillCharToUse
             << std::setw(0)
             << std::setfill('\0');
    }

    // Write the sign characters.
    if (specifiers.sign == FORMAT_SIGN_POSITIVE_SPACE_TOGGLE && value >= 0) {
        ostr << ' ';
    }
    else if (specifiers.sign == FORMAT_SIGN_POSITIVE_SPACE_TOGGLE && value < 0 && specifiers.align == FORMAT_ALIGN_INTERNAL) {
        ostr << '-';
    }

    // Add the center padding if any.
    if (addPadding && paddingCenter > 0) {
        ostr << std::setw(paddingCenter)
             << std::setfill(fillCharToUse)
             << fillCharToUse
             << std::setw(0)
             << std::setfill('\0');
    }

    // If alternate form, prefix with the base.
    if (specifiers.alternateForm) {
        switch (specifiers.type) {
            case FORMAT_BINARY_TOGGLE:
                ostr << "0b";
                break;

            case FORMAT_OCTAL_TOGGLE:
                ostr << "0o";
                break;

            case FORMAT_LOWERCASE_HEX_TOGGLE:
                ostr << "0x";
                break;

            case FORMAT_UPPERCASE_HEX_TOGGLE:
                ostr << "0X";
                break;

            default:
                // Ignore unknown alternate forms
                break;
        }
    }

    // Write the value.
    ostr << writtenString;

    if (specifiers.type == FORMAT_PERCENTAGE_MODE) {
        ostr << FORMAT_PERCENTAGE_MODE;
    }

    // Add the right padding.
    if (addPadding && paddingRight > 0) {
        ostr << std::setw(paddingRight)
             << std::setfill(fillCharToUse)
             << fillCharToUse;
    }
}


/**
 * This function prepares the output stream for outputting the integer format from the format specifiers in
 * @p specifiers.
 *
 * When calling this function make sure to check the value of @p useValue, if this is false then do not print the
 * value to the stream as this has already been done.
 *
 * @param[in]     specifiers  The format specifiers, this is the parsed format options.
 * @param[in,out] ostr  The stream to configure, and to use for output.
 * @param[in,out] value  The value to format and output.
 * @param[out]    useValue  A boolean value of true or false, specifying whether to write the value or not, if this is
 *                set to false, the value should not be written by the caller, because it is already written.
 */
void PreprocessStreamForInteger(const BasicFormatSpecifiers& specifiers, std::ostream& ostr, int64_t& value,
        bool& useValue)
{
    // By default use the value.
    useValue = true;

    // Set the minimum width of the output
    ostr << std::setw(specifiers.width);

    // Set the fill character if it needs to be set
    if (specifiers.fill) {
        ostr << std::setfill(specifiers.fill);
    }

    // If thousands separator is set imbue a locale, since the PEP-3101 dictates that the thousands separator should
    // be a ',' we use a custom numpunct class reporting the thousands separator as a comma.
    if (specifiers.thousandSeparator) {
        std::locale loc(ostr.getloc(), new AlwaysCommaAsThousandsSep);
        ostr.imbue(loc);
    }

    switch (specifiers.sign) {
        // Nothing really needs to be done for this, this is default behavior, show a sign character for negatives
        // only.
        case FORMAT_SIGN_NEGATIVES_TOGGLE:
            break;

        // When setting the sign character to +, numbers will always have a sign of either + or -. Negative numbers
        // will have a - sign, and 0 and positive numberes will have a +.
        case FORMAT_SIGN_ALWAYS_TOGGLE:
            ostr << std::showpos;
            break;

        // Like with +, this will always show a sign character, however the sign character for 0 or positive numbers
        // will be a space. This sets the width of the stream to 0, as it requires some post processing, it is simply
        // not supported directly by the underlying stream.
        case FORMAT_SIGN_POSITIVE_SPACE_TOGGLE:
            ostr << std::setw(0);
            break;

        // Ignore unkown sign character settings.
        default:
            break;
    }

    switch (specifiers.align) {
        // Will align the value to the left:
        // [-xxxx    ]
        case FORMAT_ALIGN_LEFT:
            ostr << std::left;
            break;

        // Will align the value to the right, and the sign to the left:
        // [-    xxxx]
        case FORMAT_ALIGN_INTERNAL:
            ostr << std::internal;
            break;

        // Will center the value and sign:
        // [  -xxxx  ]
        case FORMAT_ALIGN_CENTER:
            // This needs to be processed afterwards, for now make sure we only have the raw data
            ostr << std::setw(0);
            break;

        // Will align the value to the right:
        // [    -xxxx]
        case FORMAT_ALIGN_RIGHT:
        default:
            ostr << std::right;
    }

    switch (specifiers.type) {
        // Write number as binary number (skip leading zeroes).
        case FORMAT_BINARY_TOGGLE:
            {
                // Generally this needs to be done later
                ostr << std::noshowpos;
                // Use the bitset's built in to string method
                std::bitset<64> helper{static_cast<unsigned long long>(value)};
                std::string binaryString = helper.to_string();
                // Find first significant bit, 63 since we show at least one bit
                int index = 0;
                while (index < 63 && binaryString[index] == '0') {
                    ++index;
                }
                ostr << (binaryString.c_str() + index);
                // Since we have already output the value, let's not do it again
                useValue = false;
            };
            break;

        // Normally this would print the unicode character corresponding to the number, but this has been left out for
        // now, it might be introduced at some later point
        case FORMAT_UNICODE_CHAR_TOGGLE:
            break;

        // Print the number in octal format
        case FORMAT_OCTAL_TOGGLE:
            ostr << std::oct;
            break;

        // Print the number in hex format using upper case letters (A-Z) and apparently (found out through tests in
        // Python 3.3.2) also upper case 0X.
        case FORMAT_UPPERCASE_HEX_TOGGLE:
            // Hex format
            ostr << std::uppercase;
            /* no break */
        case FORMAT_LOWERCASE_HEX_TOGGLE:
            ostr << std::hex;
            break;

        // Print the number in the localized version, using the thousands separator character of the locale.
        case FORMAT_LOCALIZED_NUMBER_TOGGLE:
            ostr.imbue(std::locale(""));
            break;

        // Ordinary integer number
        case FORMAT_NORMAL_NUMBER_TOGGLE:
        default:
            break;
    }
}


/**
 * Post processing written text to the output stream.
 *
 * Prints the written string in the following format to the ostr stream:
 *
 * [left padding][sign][type][fill][written string][right padding]
 *
 * @param[in] writtenString  The actual string that should be written to the output string if no post processing is
 *            needed.
 * @param[in] specifiers  The format specifiers used for this
 * @param[out] ostr  The output string to write the finalized string to.
 * @param[in] value  The value that should be formatted and output.
 */
void PostprocessStreamForInteger(std::string&& writtenString, const BasicFormatSpecifiers& specifiers,
        std::ostream& ostr, const int64_t& value)
{
    int contentWidth = static_cast<int>(writtenString.size());
    bool addPadding = (specifiers.sign == FORMAT_SIGN_POSITIVE_SPACE_TOGGLE || specifiers.align == FORMAT_ALIGN_CENTER);
    char fillCharToUse = specifiers.fill ? specifiers.fill : ' ';

    // If the sign is a space, we should make room for it in the calculations, it has not been added to the
    // writtenString yet.
    if (specifiers.sign == FORMAT_SIGN_POSITIVE_SPACE_TOGGLE && value >= 0) {
        ++contentWidth;
    }
    else if (specifiers.sign == FORMAT_SIGN_POSITIVE_SPACE_TOGGLE && value < 0
            && specifiers.align == FORMAT_ALIGN_INTERNAL) {
        // Remove sign, we do not need to add to contentWidth here since the sign was included.
        writtenString.erase(0, 1);
    }

    // If the number should be prefixed with the base (0b, 0o, 0x, 0X) make room for it.
    if (specifiers.alternateForm && strchr("boxX", specifiers.type) != nullptr) {
        contentWidth += 2;
    }

    // Calculate the paddings: [left][sign][center][value][right]
    int paddingLeft = 0;
    int paddingCenter = 0;
    int paddingRight = 0;
    if (addPadding) {
        switch (specifiers.align) {
            case FORMAT_ALIGN_LEFT:
                paddingRight = specifiers.width - contentWidth;
                break;

            case FORMAT_ALIGN_CENTER:
                paddingLeft = (specifiers.width - contentWidth) / 2;
                paddingRight = specifiers.width - paddingLeft - contentWidth;
                break;

            case FORMAT_ALIGN_INTERNAL:
                paddingCenter = specifiers.width - contentWidth;
                break;

            case FORMAT_ALIGN_RIGHT:
            default:
                paddingLeft = specifiers.width - contentWidth;
                break;
        }
    }

    // Add the left padding if any.
    if (addPadding && paddingLeft > 0) {
        ostr << std::setw(paddingLeft)
             << std::setfill(fillCharToUse)
             << fillCharToUse
             << std::setw(0)
             << std::setfill('\0');
    }

    // Write the sign characters.
    if (specifiers.sign == FORMAT_SIGN_POSITIVE_SPACE_TOGGLE && value >= 0) {
        ostr << ' ';
    }
    else if (specifiers.sign == FORMAT_SIGN_POSITIVE_SPACE_TOGGLE && value < 0 && specifiers.align == FORMAT_ALIGN_INTERNAL) {
        ostr << '-';
    }

    // Add the center padding if any.
    if (addPadding && paddingCenter > 0) {
        ostr << std::setw(paddingCenter)
             << std::setfill(fillCharToUse)
             << fillCharToUse
             << std::setw(0)
             << std::setfill('\0');
    }

    // If alternate form, prefix with the base.
    if (specifiers.alternateForm) {
        switch (specifiers.type) {
            case FORMAT_BINARY_TOGGLE:
                ostr << "0b";
                break;

            case FORMAT_OCTAL_TOGGLE:
                ostr << "0o";
                break;

            case FORMAT_LOWERCASE_HEX_TOGGLE:
                ostr << "0x";
                break;

            case FORMAT_UPPERCASE_HEX_TOGGLE:
                ostr << "0X";
                break;

            default:
                // Ignore unknown alternate forms.
                break;
        }
    }

    // Write the value.
    ostr << writtenString;

    // Add the right padding.
    if (addPadding && paddingRight > 0) {
        ostr << std::setw(paddingRight)
             << std::setfill(fillCharToUse)
             << fillCharToUse;
    }
}


/**
 * Sets up the output stream for outputting a formatted string.
 *
 * This function sets up the stream to comply with the format specifiers in parameter @p specifiers so that the value
 * output follows the desired formatting.  Not all formatting can be done within the function, these will be handled
 * by the PostprocessStreamForString method.
 *
 * @param[in] specifiers  The format specifiers to use for formatting this string.
 * @param[out] ostr  The output stream to to format and use for formatted output, note that the actual string is not
 *             written to the stream yet.
 * @param[in] unnamed  This parameter is not used by this function, and is as such unnamed.
 * @param[out] useValue  Indicates whether or not the outer function should write the value to the stream, this is
 *             always true, indicating that it should.
 */
void PreprocessStreamForString(const BasicFormatSpecifiers& specifiers, std::ostream& ostr, const char*,
        bool& useValue)
{
    // By default use the value.
    useValue = true;

    // Set the minimum width of the output
    ostr << std::setw(specifiers.width);

    // Set the fill character if it needs to be set
    if (specifiers.fill) {
        ostr << std::setfill(specifiers.fill);
    }

    switch (specifiers.align) {
        // Will align the value to the right:
        // [    xxxxx]
        case FORMAT_ALIGN_RIGHT:
            ostr << std::right;
            break;

        // Will align the value to the right, and the sign to the left, however sign is not really used in strings so
        // it does not really produce any difference from right aligning text, Python does not allow this for strings:
        // [    xxxxx]
        case FORMAT_ALIGN_INTERNAL:
            ostr << std::internal;
            break;

        // Will center the text:
        // [  xxxxx  ]
        case FORMAT_ALIGN_CENTER:
            // This needs to be processed afterwards, for now make sure we only have the raw data
            ostr << std::setw(0);
            break;

        // Will align the value to the left (default for strings):
        // [xxxxx    ]
        case FORMAT_ALIGN_LEFT:
        default:
            ostr << std::left;
    }
}


/**
 * Post processing written text to the output stream.
 *
 * Prints the written string in the following format to the ostr stream:
 *
 * [left padding][sign][type][fill][written string][right padding]
 *
 * @param[in] writtenString  The actual string that should be written to the output string if no post processing is
 *            needed.
 * @param[in] specifiers  The format specifiers used for this
 * @param[out] ostr  The output string to write the finalized string to.
 * @param[in] unnamed  This parameter is not used here.
 */
void PostprocessStreamForString(std::string&& writtenString, const BasicFormatSpecifiers& specifiers,
        std::ostream& ostr, const char*)
{
    int contentWidth = static_cast<int>(writtenString.size());
    bool addPadding = specifiers.align == FORMAT_ALIGN_CENTER;
    char fillCharToUse = specifiers.fill ? specifiers.fill : ' ';

    // Calculate the paddings: [left][value][right]
    int paddingLeft = 0;
    int paddingRight = 0;

    if (specifiers.precision > 0 && specifiers.precision < contentWidth) {
        writtenString = writtenString.substr(0, static_cast<std::string::size_type>(specifiers.precision));
        contentWidth = specifiers.precision;
        if (specifiers.width > specifiers.precision) {
            addPadding = true;
        }
    }

    if (addPadding) {
        switch (specifiers.align) {
            case FORMAT_ALIGN_LEFT:
                paddingRight = specifiers.width - contentWidth;
                break;

            case FORMAT_ALIGN_CENTER:
                paddingLeft = (specifiers.width - contentWidth) / 2;
                paddingRight = specifiers.width - paddingLeft - contentWidth;
                break;

            case FORMAT_ALIGN_INTERNAL:
            case FORMAT_ALIGN_RIGHT:
            default:
                paddingLeft = specifiers.width - contentWidth;
                break;
        }
    }

    // Add the left padding if any.
    if (addPadding && paddingLeft > 0) {
        ostr << std::setw(paddingLeft)
             << std::setfill(fillCharToUse)
             << fillCharToUse
             << std::setw(0)
             << std::setfill('\0');
    }

    // Write the value.
    ostr << writtenString;

    // Add the right padding.
    if (addPadding && paddingRight > 0) {
        ostr << std::setw(paddingRight)
             << std::setfill(fillCharToUse)
             << fillCharToUse;
    }
}

}

namespace utils {
namespace str {


/**
 * The constructor, constructing an IllegalFormatStringException for a specific format string, for an error
 * occurring at position @p pos with the error message: @p message
 *
 * @param[in] formatStr  The format string causing the exception to be thrown.
 * @param[in] pos  The position in the format string where the problem was detected.
 * @param[in] message  The message describing the problem found.
 */
IllegalFormatStringException::IllegalFormatStringException(const std::string& formatStr, int pos, const std::string& message)
    : std::runtime_error(message)
{
    this->formatString = formatStr;
    this->errorPosition = pos;
    this->message = message;

    std::stringstream errorDescription;

    errorDescription << "Invalid string format, error at position: " << errorPosition << std::endl;
    errorDescription << formatString << std::endl;
    errorDescription << std::setfill(' ') << std::setw(errorPosition) << "" << std::setfill('\0') << std::setw(0) << "^" << std::endl;
    errorDescription << message << std::endl;

    this->fullDescription = errorDescription.str();
}


IllegalFormatStringException::~IllegalFormatStringException() noexcept
{
    // Nothing to do here...
}


/**
 * Returns the position in the format string where the error was detected.
 *
 * The position returned i 0 based, so a value of 0 indicates the first character in the format string.
 *
 * @return Returns the position in the format string where the error was detected.
 */
int IllegalFormatStringException::GetErrorPosition() const noexcept
{
    return this->errorPosition;
}


/**
 * Returns the format string that is illegal.
 *
 * @return Returns the format string that is illegal.
 */
const char* IllegalFormatStringException::GetFormatString() const noexcept
{
    return this->formatString.c_str();
}


/**
 * Returns the error message describing the error occurring from parsing the format string.
 *
 * @return Returns the error message describing the error occurring from parsing the format string.
 */
const char* IllegalFormatStringException::GetMessage() const noexcept
{
    return this->message.c_str();
}


/**
 * Returns a complete descriptive string about the error occurred.
 *
 * @return Returns a complete descriptive string about the error occurred.
 */
const char* IllegalFormatStringException::what() const noexcept
{
    return this->fullDescription.c_str();
}


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
void ConvertToBasicFormatSpecifiers(const char* formatParameter, BasicFormatSpecifiers& specifiers, int& pos)
{
    if (formatParameter && formatParameter[pos]) {
        ReadAlignSpecifier(formatParameter, specifiers, pos);
        ReadSignSpecifier(formatParameter, specifiers, pos);
        ReadAlternateSpecifier(formatParameter, specifiers, pos);
        ReadSignAwareZeroSpecifier(formatParameter, specifiers, pos);
        ReadWidthSpecifier(formatParameter, specifiers, pos);
        ReadThousandSepSpecifier(formatParameter, specifiers, pos);
        ReadPrecisionSpecifier(formatParameter, specifiers, pos);
        ReadTypeSpecifier(formatParameter, specifiers, pos);
    }
}


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
void FormatType(short value, const char* formatSpecifier, std::ostream& output)
{
    FormatType(static_cast<int64_t>(value), formatSpecifier, output);
}


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
void FormatType(int value, const char* formatSpecifier, std::ostream& output)
{
    FormatType(static_cast<int64_t>(value), formatSpecifier, output);
}


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
void FormatType(int64_t value, const char* formatSpecifier, std::ostream& output)
{
    int pos = 0;
    BasicFormatSpecifiers specifiers;
    InitializeFormatSpecifier(specifiers);
    ConvertToBasicFormatSpecifiers(formatSpecifier, specifiers, pos);

    bool useValue = true;
    std::stringstream buffer;
    PreprocessStreamForInteger(specifiers, buffer, value, useValue);
    if (useValue) {
        buffer << value;
    }
    PostprocessStreamForInteger(buffer.str(), specifiers, output, value);
}


/**
 * Formatting function for the primitive type bool, this will be called by the format function and can be called as
 * is to format a boolean from a specific format specifier.
 *
 * To the formatter all numbers of the type: int, long, int64, and bool are treated equally, thus the corresponding
 * format functions converts their respective @p value to an int64 and calls the corresponding FormatType function.
 *
 * The format of the format specifier string can be found in Python PEP-3101, where the format specifier is described
 * https://www.python.org/dev/peps/pep-3101/
 *
 * @param value[in]  The value to format.
 * @param formatSpecifier[in]  The format specifier to use.
 * @param output[out]  A reference to an output stream to write the formatted result to.
 */
void FormatType(bool value, const char* formatSpecifier, std::ostream& output)
{
    FormatType(static_cast<int64_t>(value), formatSpecifier, output);
}


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
void FormatType(float value, const char* formatSpecifier, std::ostream& output)
{
    FormatType(static_cast<long double>(value), formatSpecifier, output);
}


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
void FormatType(double value, const char* formatSpecifier, std::ostream& output)
{
    FormatType(static_cast<long double>(value), formatSpecifier, output);
}


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
void FormatType(long double value, const char* formatSpecifier, std::ostream& output)
{
    int pos = 0;
    BasicFormatSpecifiers specifiers;
    InitializeFormatSpecifier(specifiers);
    ConvertToBasicFormatSpecifiers(formatSpecifier, specifiers, pos);

    bool useValue = true;
    bool useDynamic = true;
    std::streamsize minPrecision = DOUBLE_MIN_DEFAULT_PRECISION;
    std::streamsize maxPrecision = DOUBLE_MAX_DEFAULT_PRECISION;
    std::streamsize scientificCeil = 0;
    std::stringstream buffer;
    PreprocessStreamForDecimal(specifiers, buffer, value, useValue, useDynamic, minPrecision, maxPrecision, scientificCeil);
    if (useValue) {
        if (useDynamic && specifiers.precision == PRECISION_NOT_SET) {
            FormatDynamicDecimal fdd(value, minPrecision, maxPrecision, scientificCeil);
            buffer << fdd;
        }
        else {
            buffer << value;
        }
    }
    PostprocessStreamForDecimal(buffer.str(), specifiers, output, value);
}


/**
 * Formatting function for the std class string, this will be called by the format function and can be called as
 * is to format a string from a specific format specifier.
 *
 * To the formatter all strings of the type: char* and string are treated equally, thus the corresponding format
 * functions converts their respective @p value to a const char* and calls the function FormatType for type const char*
 * function.
 *
 * The format of the format specifier string can be found in Python PEP-3101, where the format specifier is described
 * https://www.python.org/dev/peps/pep-3101/
 *
 * @param value[in]  The value to format.
 * @param formatSpecifier[in]  The format specifier to use.
 * @param output[out]  A reference to an output stream to write the formatted result to.
 */
void FormatType(const std::string& value, const char* formatSpecifier, std::ostream& output)
{
    FormatType(value.c_str(), formatSpecifier, output);
}


/**
 * Formatting function for the primitive type char*, this will be called by the format function and can be called as
 * is to format a string from a specific format specifier.
 *
 * To the formatter all strings of the type: char* and string are treated equally, thus the corresponding format
 * functions converts their respective @p value to a const char* and calls this function.
 *
 * The format of the format specifier string can be found in Python PEP-3101, where the format specifier is described
 * https://www.python.org/dev/peps/pep-3101/
 *
 * @param value[in]  The value to format.
 * @param formatSpecifier[in]  The format specifier to use.
 * @param output[out]  A reference to an output stream to write the formatted result to.
 */
void FormatType(const char* value, const char* formatSpecifier, std::ostream& output)
{
    int pos = 0;
    BasicFormatSpecifiers specifiers;
    InitializeFormatSpecifier(specifiers);
    ConvertToBasicFormatSpecifiers(formatSpecifier, specifiers, pos);

    bool useValue = true;
    std::stringstream buffer;
    PreprocessStreamForString(specifiers, buffer, value, useValue);
    if (useValue) {
        buffer << value;
    }
    PostprocessStreamForString(buffer.str(), specifiers, output, value);
}


/**
 * Converts the short value to a different type specified by the format string, if the conversion was done and
 * formatted within this scope the function returns true, otherwise the function returns false, and nothing will have
 * been output.
 *
 * @param[in]  value  The value we wish to output.
 * @param[in]  convertToType  The type we wish to convert to before outputting, this can be any of type s, r and d.
 * @param[in]  formatSpecifier  The format specifier to send to the converted result.
 * @param[out]  output  The output stream, to write the formatted output to.
 *
 * @return Returns true if the type was converted and formatted, otherwise false is returned, meaning formatting is
 *         required elsewhere.
 */
bool ConvertAndFormatType(short value, FormatFragment& fragment, std::ostream& output)
{
    return ConvertAndFormatType(static_cast<int64_t>(value), fragment, output);
}


/**
 * Converts the int value to a different type specified by the format string, if the conversion was done and
 * formatted within this scope the function returns true, otherwise the function returns false, and nothing will have
 * been output.
 *
 * @param[in]  value  The value we wish to output.
 * @param[in]  convertToType  The type we wish to convert to before outputting, this can be any of type s, r and d.
 * @param[in]  formatSpecifier  The format specifier to send to the converted result.
 * @param[out]  output  The output stream, to write the formatted output to.
 *
 * @return Returns true if the type was converted and formatted, otherwise false is returned, meaning formatting is
 *         required elsewhere.
 */
bool ConvertAndFormatType(int value, FormatFragment& fragment, std::ostream& output)
{
    return ConvertAndFormatType(static_cast<int64_t>(value), fragment, output);
}


/**
 * Converts the int64 value to a different type specified by the format string, if the conversion was done and
 * formatted within this scope the function returns true, otherwise the function returns false, and nothing will have
 * been output.
 *
 * @param[in]  value  The value we wish to output.
 * @param[in]  convertToType  The type we wish to convert to before outputting, this can be any of type s, r and d.
 * @param[in]  formatSpecifier  The format specifier to send to the converted result.
 * @param[out]  output  The output stream, to write the formatted output to.
 *
 * @return Returns true if the type was converted and formatted, otherwise false is returned, meaning formatting is
 *         required elsewhere.
 */
bool ConvertAndFormatType(int64_t value, FormatFragment& fragment, std::ostream& output)
{
    if (!fragment.selectors.empty()) {
        std::string selector = fragment.selectors.front();
        fragment.selectors.pop();

        if (selector == "abs") {
            return ConvertAndFormatType(std::abs(value), fragment, output);
        }

        if (selector == "sign") {
            return ConvertAndFormatType((value < 0ll ? -1ll : 1ll), fragment, output);
        }

        if (selector == "inc") {
            return ConvertAndFormatType(value + 1, fragment, output);
        }

        if (selector == "dec") {
            return ConvertAndFormatType(value - 1, fragment, output);
        }

        if (selector == "sqrt") {
            return ConvertAndFormatType(std::sqrt(value), fragment, output);
        }
    }

    switch (fragment.explicitConversion) {
        case 's':
        case 'r':
            FormatType(std::to_string(value), fragment.formatSpecifier, output);
            break;

        case 'd':
            FormatType(static_cast<long double>(value), fragment.formatSpecifier, output);
            break;

        default:
            FormatType(value, fragment.formatSpecifier, output);
            break;
    }

    return true;
}


/**
 * Converts the float value to a different type specified by the format string, if the conversion was done and
 * formatted within this scope the function returns true, otherwise the function returns false, and nothing will have
 * been output.
 *
 * @param[in]  value  The value we wish to output.
 * @param[in]  convertToType  The type we wish to convert to before outputting, this can be any of type s, r and i.
 * @param[in]  formatSpecifier  The format specifier to send to the converted result.
 * @param[out]  output  The output stream, to write the formatted output to.
 *
 * @return Returns true if the type was converted and formatted, otherwise false is returned, meaning formatting is
 *         required elsewhere.
 */
bool ConvertAndFormatType(float value, FormatFragment& fragment, std::ostream& output)
{
    return ConvertAndFormatType(static_cast<long double>(value), fragment, output);
}


/**
 * Converts the double value to a different type specified by the format string, if the conversion was done and
 * formatted within this scope the function returns true, otherwise the function returns false, and nothing will have
 * been output.
 *
 * @param[in]  value  The value we wish to output.
 * @param[in]  convertToType  The type we wish to convert to before outputting, this can be any of type s, r and i.
 * @param[in]  formatSpecifier  The format specifier to send to the converted result.
 * @param[out]  output  The output stream, to write the formatted output to.
 *
 * @return Returns true if the type was converted and formatted, otherwise false is returned, meaning formatting is
 *         required elsewhere.
 */
bool ConvertAndFormatType(double value, FormatFragment& fragment, std::ostream& output)
{
    return ConvertAndFormatType(static_cast<long double>(value), fragment, output);
}


/**
 * Converts the long double value to a different type specified by the format string, if the conversion was done and
 * formatted within this scope the function returns true, otherwise the function returns false, and nothing will have
 * been output.
 *
 * @param[in]  value  The value we wish to output.
 * @param[in]  convertToType  The type we wish to convert to before outputting, this can be any of type s, r and i.
 * @param[in]  formatSpecifier  The format specifier to send to the converted result.
 * @param[out]  output  The output stream, to write the formatted output to.
 *
 * @return Returns true if the type was converted and formatted, otherwise false is returned, meaning formatting is
 *         required elsewhere.
 */
bool ConvertAndFormatType(long double value, FormatFragment& fragment, std::ostream& output)
{
    switch (fragment.explicitConversion) {
        case 's':
        case 'r':
            FormatType(std::to_string(value), fragment.formatSpecifier, output);
            break;

        case 'i':
            FormatType(static_cast<int64_t>(value), fragment.formatSpecifier, output);
            break;

        default:
            FormatType(value, fragment.formatSpecifier, output);
            break;
    }

    return true;
}


/**
 * Converts the boolean value to a different type specified by the format string, if the conversion was done and
 * formatted within this scope the function returns true, otherwise the function returns false, and nothing will have
 * been output.
 *
 * @param[in]  value  The value we wish to output.
 * @param[in]  convertToType  The type we wish to convert to before outputting, this can be any of type s, r, i and d.
 * @param[in]  formatSpecifier  The format specifier to send to the converted result.
 * @param[out]  output  The output stream, to write the formatted output to.
 *
 * @return Returns true if the type was converted and formatted, otherwise false is returned, meaning formatting is
 *         required elsewhere.
 */
bool ConvertAndFormatType(bool value, FormatFragment& fragment, std::ostream& output)
{
    char convertToType = fragment.explicitConversion;

    // Python has a special way of handling booleans, it appears that if no explicit type conversion is chosen, and no
    // format specifier is used, that the bool is written using the string representation. We handle this by overriding
    // the convertToType to s, forcing it to be written as string.
    if (fragment.formatSpecifier.empty()) {
        convertToType = 's';
    }

    switch (convertToType) {
        case 's':
        case 'r':
            FormatType(value ? "True" : "False", fragment.formatSpecifier, output);
            break;

        case 'i':
            FormatType(value ? 1 : 0, fragment.formatSpecifier, output);
            break;

        case 'd':
            FormatType(value ? 1.0 : 0.0, fragment.formatSpecifier, output);
            break;

        default:
            FormatType(value, fragment.formatSpecifier, output);
            break;
    }

    return true;
}


/**
 * Converts the string value to a different type specified by the format string, if the conversion was done and
 * formatted within this scope the function returns true, otherwise the function returns false, and nothing will have
 * been output.
 *
 * @param[in]  value  The value we wish to output.
 * @param[in]  convertToType  The type we wish to convert to before outputting, this can be any of type i and d.
 * @param[in]  formatSpecifier  The format specifier to send to the converted result.
 * @param[out]  output  The output stream, to write the formatted output to.
 *
 * @return Returns true if the type was converted and formatted, otherwise false is returned, meaning formatting is
 *         required elsewhere.
 */
bool ConvertAndFormatType(const std::string& value, FormatFragment& fragment, std::ostream& output)
{
    return ConvertAndFormatType(value.c_str(), fragment, output);
}


/**
 * Converts the string value to a different type specified by the format string, if the conversion was done and
 * formatted within this scope the function returns true, otherwise the function returns false, and nothing will have
 * been output.
 *
 * @param[in]  value  The value we wish to output.
 * @param[in]  convertToType  The type we wish to convert to before outputting, this can be any of type i and d.
 * @param[in]  formatSpecifier  The format specifier to send to the converted result.
 * @param[out]  output  The output stream, to write the formatted output to.
 *
 * @return Returns true if the type was converted and formatted, otherwise false is returned, meaning formatting is
 *         required elsewhere.
 */
bool ConvertAndFormatType(const char* value, FormatFragment& fragment, std::ostream& output)
{
    char* dummy;

    switch (fragment.explicitConversion) {
        case 'i':
            FormatType(std::strtoll(value, &dummy, 10), fragment.formatSpecifier, output);
            break;

        case 'd':
            FormatType(std::strtold(value, &dummy), fragment.formatSpecifier, output);
            break;

        default:
            FormatType(value, fragment.formatSpecifier, output);
            break;
    }

    return true;
}


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
void ParseFormatStr(const char* formatStr, std::ostream& ostr, std::vector<FormatFragment>& fragments)
{
    int pos = 0;
    int nextParameterIndex = 0;

    // If there is any text in the beginning of the format string, send this directly to the output stream.
    GetPlainTextFragment(formatStr, ostr, pos);

    // We have handled all starting text, now handle all the format specifiers, and the text following them.
    while (formatStr[pos]) {
        ParseFormatParameter(formatStr, fragments, pos, nextParameterIndex);
    }
}


/**
 * Iterates over the fragment vector adding the vector results to the output stream.
 *
 * This method simply appends the contents of the text property of the FormatFragment struct from the parameter
 * @p fragments to the output stream from parameter @p ostr.  If the macro FORMAT_DISABLE_THROW_OUT_OF_RANGE is not
 * set, an exception will be thrown if a format parameter was not handled.
 *
 * @param[in]  fragments  A vector of format fragments to output.
 * @param[out] ostr  The output stream to write the format fragments to.
 */
void OutputFragments(const std::vector<FormatFragment>& fragments, std::ostream& ostr)
{
    for (const FormatFragment& fragment : fragments) {
#ifndef FORMAT_DISABLE_THROW_OUT_OF_RANGE
        if (!fragment.handled) {
            std::stringstream exceptionMsg;
            exceptionMsg << "Format parameter: " << fragment.index << " does not refer to a valid parameter.";
            throw std::out_of_range(exceptionMsg.str());
        }
#endif  // FORMAT_DISABLE_THROW_OUT_OF_RANGE
        ostr << fragment.text;
    }
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
std::string Format(const char* formatStr)
{
    std::stringstream ostr;
    std::vector<FormatFragment> fragments;
    ParseFormatStr(formatStr, ostr, fragments);
    OutputFragments(fragments, ostr);
    return ostr.str();
}


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
std::string Format(const std::string& formatStr)
{
    return Format(formatStr.c_str());
}

}
}
