/******************************************************************************\
* Copyright (c) 2016, Robert van Engelen, Genivia Inc. All rights reserved.    *
*                                                                              *
* Redistribution and use in source and binary forms, with or without           *
* modification, are permitted provided that the following conditions are met:  *
*                                                                              *
*   (1) Redistributions of source code must retain the above copyright notice, *
*       this list of conditions and the following disclaimer.                  *
*                                                                              *
*   (2) Redistributions in binary form must reproduce the above copyright      *
*       notice, this list of conditions and the following disclaimer in the    *
*       documentation and/or other materials provided with the distribution.   *
*                                                                              *
*   (3) The name of the author may not be used to endorse or promote products  *
*       derived from this software without specific prior written permission.  *
*                                                                              *
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED *
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF         *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO   *
* EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,       *
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, *
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;  *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,     *
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR      *
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF       *
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                   *
\******************************************************************************/

/**
@file      convert.h
@brief     RE/flex regex converter
@author    Robert van Engelen - engelen@genivia.com
@copyright (c) 2015-2017, Robert van Engelen, Genivia Inc. All rights reserved.
@copyright (c) BSD-3 License - see LICENSE.txt
*/

#ifndef REFLEX_CONVERT_H
#define REFLEX_CONVERT_H

#include <reflex/error.h>
#include <string>
#include <map>

#if (defined(__WIN32__) || defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(__BORLANDC__)) && !defined(__CYGWIN__) && !defined(__MINGW32__) && !defined(__MINGW64__)
# pragma warning( disable : 4290 )
#endif

namespace reflex {

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  Regex convert                                                             //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

/// Conversion flags for reflex::convert.
typedef int convert_flag_type;

namespace convert_flag {
  const convert_flag_type none      = 0x00; ///< no conversion (default)
  const convert_flag_type unicode   = 0x01; ///< convert . (dot), `\s`, `\w`, `\l`, `\u`, `\S`, `\W`, `\L`, `\U` to Unicode
  const convert_flag_type recap     = 0x02; ///< remove capturing groups, add capturing groups to the top level
  const convert_flag_type lex       = 0x04; ///< convert Lex/Flex regular expression syntax
  const convert_flag_type u4        = 0x08; ///< convert `\uXXXX` and UTF-16 surrogate pairs
  const convert_flag_type anycase   = 0x10; ///< convert regex to ignore case, same as `(?i)`
  const convert_flag_type multiline = 0x20; ///< regex with multiline anchors `^` and `$`, same as `(?m)`
  const convert_flag_type dotall    = 0x40; ///< convert `.` (dot) to match all, same as `(?s)`
  const convert_flag_type freespace = 0x80; ///< convert regex by removing spacing, same as `(?x)`
};

/// @brief Returns the converted regex string given a regex library signature and conversion flags, throws regex_error.
///
/// A regex library signature is a string of the form `"decls:escapes?+."`.
///
/// The optional `"decls:"` part specifies which modifiers and other special `(?...)` constructs are supported:
/// - non-capturing group `(?:...)` is supported
/// - one or all of "imsx" specify which (?ismx:...) modifiers are supported
/// - `#` specifies that `(?#...)` comments are supported
/// - `=` specifies that `(?=...)` lookahead is supported
/// - `<` specifies that `(?<...)` lookbehind is supported
/// - `!` specifies that `(?!=...)` and `(?!<...)` are supported
/// - `^` specifies that `(?^...)` negative (reflex) patterns are supported
///
/// The `"escapes"` characters specify which standard escapes are supported:
/// - `a` for `\a` (BEL U+0007)
/// - `b` for `\b` (BS U+0008) in brackets `[\b]` only AND the `\b` word boundary
/// - `c` for `\cX` control character specified by `X` modulo 32
/// - `d` for `\d` ASCII digit `[0-9]`
/// - `e` for `\e` ESC U+001B
/// - `f` for `\f` FF U+000C
/// - `h` for `\h` ASCII blank `[ \t]` (SP U+0020 or TAB U+0009)
/// - `i` for `\i` reflex indent boundary
/// - `j` for `\j` reflex dedent boundary
/// - `l` for `\l` ASCII lower case letter `[a-z]`
/// - `n` for `\n` LF U+000A
/// - `p` for `\p{C}` ASCII POSIX character class specified by `C`
/// - `r` for `\r` CR U+000D
/// - `s` for `\s` space (SP, TAB, LF, VT, FF, or CR)
/// - `t` for `\t` TAB U+0009
/// - `u` for `\u` ASCII upper case letter `[A-Z]` (when not followed by `{XXXX}`)
/// - `v` for `\v` VT U+000B
/// - `w` for `\w` ASCII word-like character `[0-9A-Z_a-z]`
/// - `x` for `\xXX` 8-bit character encoding in hexadecimal
/// - `y` for `\y` word boundary
/// - `z` for `\z` end of input anchor
/// - `0` for `\0nnn` 8-bit character encoding in octal requires a leading `0`
/// - ``` for `\`` begin of input anchor
/// - `'` for `\'` end of input anchor
/// - `<` for `\<` left word boundary
/// - `>` for `\>` right word boundary
/// - `A` for `\A` begin of input anchor
/// - `B` for `\B` non-word boundary
/// - `D` for `\D` ASCII non-digit `[^0-9]`
/// - `H` for `\H` ASCII non-blank `[^ \t]`
/// - `L` for `\L` ASCII non-lower case letter `[^a-z]`
/// - `P` for `\P{C}` ASCII POSIX inverse character class specified by `C`
/// - `Q` for `\Q...\E` quotations
/// - `S` for `\S` ASCII non-space (no SP, TAB, LF, VT, FF, or CR)
/// - `U` for `\U` ASCII non-upper case letter `[^A-Z]`
/// - `W` for `\W` ASCII non-word-like character `[^0-9A-Z_a-z]`
/// - `Z` for `\Z` end of input anchor, before the final line break
///
/// The optional `"?+"` specify lazy and possessive support:
/// - `?` lazy quantifiers for repeats are supported
/// - `+` possessive quantifiers for repeats are supported
///
/// The optional `"."` (dot) specifies that dot matches any character except newline.
std::string convert(
    const char                              *pattern,                    ///< regex string pattern to convert
    const char                              *signature = NULL,           ///< regex library signature
    convert_flag_type                        flags = convert_flag::none, ///< conversion flags
    const std::map<std::string,std::string> *macros = NULL)              ///< {name} macros to expand
  throw (regex_error);

inline std::string convert(
    const std::string&                       pattern,
    const char                              *signature = NULL,
    convert_flag_type                        flags = convert_flag::none,
    const std::map<std::string,std::string> *macros = NULL)
  throw (regex_error)
{
  return convert(pattern.c_str(), signature, flags, macros);
}

} // namespace reflex

#endif
