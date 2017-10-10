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
@file      absmatcher.h
@brief     RE/flex abstract matcher base class and pattern matcher class
@author    Robert van Engelen - engelen@genivia.com
@copyright (c) 2015-2017, Robert van Engelen, Genivia Inc. All rights reserved.
@copyright (c) BSD-3 License - see LICENSE.txt
*/

#ifndef REFLEX_ABSMATCHER_H
#define REFLEX_ABSMATCHER_H

/// This compile-time option speeds up buffer reallocation.
//#define WITH_REALLOC
/// This compile-time option speeds up matching, but slows input().
#define WITH_FAST_GET

#include <reflex/convert.h>
#include <reflex/debug.h>
#include <reflex/input.h>
#include <reflex/traits.h>
#include <cstdlib>
#include <cctype>
#include <iterator>

namespace reflex {

/// Check ASCII word-like character `[A-Za-z0-9_]`, permitting the character range 0..303 (0x12F) and EOF.
inline int isword(int c) ///< Character to check
  /// @returns nonzero if argument c is in `[A-Za-z0-9_]`, zero otherwise.
{
  return std::isalnum(static_cast<unsigned char>(c)) | (c == '_');
}

/// The abstract matcher base class template defines an interface for all pattern matcher engines.
/**
The buffer expands when matches do not fit.  The buffer size is initially 2*BLOCK size.

```
      _________________
     |  |    |    |    |
buf_=|  |text|rest|free|
     |__|____|____|____|
        ^    ^    ^    ^
        cur_ pos_ end_ max_

buf_ // points to buffered input, grows to fit long matches
cur_ // current position in buf_ while matching text, cur_ = pos_ afterwards, can be changed by more()
pos_ // position in buf_ to start the next match
end_ // position in buf_ that is free to fill with more input
max_ // allocated size of buf_, must ensure that max_ > end_ for text() to add a \0
txt_ // buf_ + cur_ points to the match, 0-terminated
len_ // length of the match
chr_ // char located at txt_[len_] when txt_[len_] is set to \0
got_ // buf_[cur_-1] character before this match (assigned before each match)
```
 
More info TODO
*/
class AbstractMatcher {
 protected:
  typedef int Method; ///< a method is one of Const::SCAN, Const::FIND, Const::SPLIT, Const::MATCH
  /// AbstractMatcher::Const common constants.
  struct Const {
    static const Method SCAN  = 0;      ///< AbstractMatcher::match method is to scan input (tokenizer)
    static const Method FIND  = 1;      ///< AbstractMatcher::match method is to find pattern in input
    static const Method SPLIT = 2;      ///< AbstractMatcher::match method is to split input at pattern matches
    static const Method MATCH = 3;      ///< AbstractMatcher::match method is to match the entire input
    static const int NUL      = '\0';   ///< NUL string terminator
    static const int UNK      = 256;    ///< unknown/undefined character meta-char marker
    static const int BOB      = 257;    ///< begin of buffer meta-char marker
    static const int EOB      = EOF;    ///< end of buffer meta-char marker
    static const size_t EMPTY = 0xFFFF; ///< accept() returns empty last split at end of input
    static const size_t BLOCK = 4096;   ///< buffer growth factor, buffer is initially 2*BLOCK size
  };
  /// AbstractMatcher::Options for matcher engines.
  struct Option {
    Option()
      :
        A(false),
        N(false),
        T(8)
    { }
    bool A; ///< accept any/all (?^X) negative patterns
    bool N; ///< nullable, find may return empty match (N/A to scan, split, matches)
    char T; ///< tab size, must be a power of 2, default is 8, for indent \i and \j
  };
  /// AbstractMatcher::Iterator class for scanning, searching, and splitting input character sequences.
  template<typename T> /// @tparam <T> AbstractMatcher or const AbstractMatcher
  class Iterator : public std::iterator<std::input_iterator_tag,T> {
    friend class AbstractMatcher;
    friend class Iterator<typename reflex::TypeOp<T>::ConstType>;
    friend class Iterator<typename reflex::TypeOp<T>::NonConstType>;
   public:
    /// Construct an AbstractMatcher::Iterator such that Iterator() == AbstractMatcher::Operation(*this, method).end().
    Iterator()
      :
        matcher_(NULL)
    { }
    /// Copy constructor.
    Iterator(const Iterator<typename reflex::TypeOp<T>::NonConstType>& it)
      :
        matcher_(it.matcher_),
        method_(it.method_)
    { }
    /// AbstractMatcher::Iterator dereference.
    T& operator*() const
      /// @returns (const) reference to the iterator's matcher.
    {
      return *matcher_;
    }
    /// AbstractMatcher::Iterator pointer.
    T* operator->() const
      /// @returns (const) pointer to the iterator's matcher.
    {
      return matcher_;
    }
    /// AbstractMatcher::Iterator equality.
    bool operator==(const Iterator<typename reflex::TypeOp<T>::ConstType>& rhs) const
      /// @returns true if iterator equals RHS.
    {
      return matcher_ == rhs.matcher_;
    }
    /// AbstractMatcher::Iterator inequality.
    bool operator!=(const Iterator<typename reflex::TypeOp<T>::ConstType>& rhs) const
      /// @returns true if iterator does not equal RHS.
    {
      return matcher_ != rhs.matcher_;
    }
    /// AbstractMatcher::Iterator preincrement.
    Iterator& operator++()
      /// @returns reference to this iterator.
    {
      if (matcher_->match(method_) == 0)
        matcher_ = NULL;
      return *this;
    }
    /// AbstractMatcher::Iterator postincrement.
    Iterator operator++(int)
      /// @returns iterator to current match.
    {
      Iterator it = *this;
      operator++();
      return it;
    }
    /// Construct an AbstractMatcher::Iterator to scan, search, or split an input character sequence.
    Iterator(
        AbstractMatcher *matcher, ///< iterate over pattern matches with this matcher
        Method           method)  ///< match using method Const::SCAN, Const::FIND, or Const::SPLIT
      :
        matcher_(matcher),
        method_(method)
    {
      if (matcher_)
      {
        matcher_->reset();
        if (matcher_->match(method_) == 0)
          matcher_ = NULL;
      }
    }
   private:
    AbstractMatcher *matcher_; ///< the matcher used by this iterator
    Method           method_;  ///< the method for pattern matching by this iterator's matcher
  };
 public:
  typedef AbstractMatcher::Iterator<AbstractMatcher>       iterator;       ///< std::input_iterator for scanning, searching, and splitting input character sequences
  typedef AbstractMatcher::Iterator<const AbstractMatcher> const_iterator; ///< std::input_iterator for scanning, searching, and splitting input character sequences
  /// AbstractMatcher::Operation functor to match input to a pattern, also provides a (const) AbstractMatcher::iterator to iterate over matches.
  class Operation {
    friend class AbstractMatcher;
   public:
    /// AbstractMatcher::Operation() matches input to a pattern using method Const::SCAN, Const::FIND, or Const::SPLIT.
    size_t operator()() const
      /// @returns value of accept() >= 1 for match or 0 for end of matches.
    {
      return matcher_->match(method_);
    }
    /// AbstractMatcher::Operation.begin() returns a std::input_iterator to the start of the matches.
    iterator begin() const
      /// @returns input iterator.
    {
      return iterator(matcher_, method_);
    }
    /// AbstractMatcher::Operation.end() returns a std::input_iterator to the end of matches.
    iterator end() const
      /// @returns input iterator.
    {
      return iterator();
    }
    /// AbstractMatcher::Operation.cbegin() returns a const std::input_iterator to the start of the matches.
    const_iterator cbegin() const
      /// @returns input const_iterator.
    {
      return const_iterator(matcher_, method_);
    }
    /// AbstractMatcher::Operation.cend() returns a const std::input_iterator to the end of matches.
    const_iterator cend() const
      /// @returns input const_iterator.
    {
      return const_iterator();
    }
   private:
    /// Construct an AbstractMatcher::Operation functor to scan, search, or split an input character sequence.
    Operation(
        AbstractMatcher *matcher, ///< use this matcher for this functor
        Method           method)  ///< match using method Const::SCAN, Const::FIND, or Const::SPLIT
      :
        matcher_(matcher),
        method_(method)
    { }
    AbstractMatcher *matcher_; ///< the matcher used by this functor
    Method           method_;  ///< the method for pattern matching by this functor's matcher
  };
 public:
  /// Reset this matcher's state to the initial state and set options (when provided).
  virtual void reset(const char *opt = NULL)
  {
    DBGLOG("AbstractMatcher::reset(%s)", opt ? opt : "(null)");
    if (opt)
    {
      opt_.A = false;
      opt_.N = false;
      opt_.T = 8;
      if (opt)
      {
        for (const char *s = opt; *s != '\0'; ++s)
        {
          switch (*s)
          {
            case 'A':
              opt_.A = true;
              break;
            case 'N':
              opt_.N = true;
              break;
            case 'T':
              opt_.T = isdigit(*(s += (s[1] == '=') + 1)) ? *s - '0' : 0;
              break;
          }
        }
      }
    }
    buf_[0] = '\0';
    txt_ = buf_;
    len_ = 0;
    cap_ = 0;
    cur_ = 0;
    pos_ = 0;
    end_ = 0;
    ind_ = 0;
    lno_ = 1;
    cno_ = 0;
    num_ = 0;
    got_ = Const::BOB;
    chr_ = '\0';
    eof_ = false;
    mat_ = false;
    blk_ = 0;
  }
  /// Set buffer block size for reading: use 1 for interactive input, 0 (or omit argument) to buffer all input in which case returns true if all the data could be read and false if a read error occurred.
  bool buffer(size_t blk = 0) ///< new block size between 1 and Const::BLOCK, or 0 to buffer all
    /// @returns true when successful to buffer all input when n=0.
  {
    if (blk > Const::BLOCK)
      blk = Const::BLOCK;
    DBGLOG("AbstractMatcher::buffer(%zu)", blk);
    blk_ = blk;
    if (blk > 0)
      return true;
    if (in.eof())
      return true;
    size_t n = in.size(); // get the (rest of the) data size, which is 0 if unknown (e.g. TTY)
    if (n > 0)
    {
      (void)grow(n + 1); // now attempt to fetch all (remaining) data to store in the buffer, +1 for a \0
      end_ = get(buf_, n);
    }
    while (in.good()) // there is more to get while good(), e.g. via wrap()
    {
      (void)grow();
      end_ += get(buf_ + end_, max_ - end_);
    }
    if (end_ == max_)
      (void)grow(1); // room for a final \0
    return in.eof();
  }
  /// Set buffer to 1 for interactive input.
  void interactive()
    /// @note Use this method before any matching is done and before any input is read since the last time input was (re)set.
  {
    DBGLOG("AbstractMatcher::interactive()");
    (void)buffer(1);
  }
  /// Flush the buffer's remaining content.
  void flush()
  {
    DBGLOG("AbstractMatcher::flush()");
    pos_ = end_;
  }
  /// Set the input character sequence for this matcher and reset/restart the matcher.
  virtual AbstractMatcher& input(const Input& input) ///< input character sequence for this matcher
    /// @returns this matcher.
  {
    DBGLOG("AbstractMatcher::input()");
    in = input;
    reset();
    return *this;
  }
  /// Returns nonzero capture index (i.e. true) if the entire input matches this matcher's pattern (and internally caches the true/false result for repeat invocations).
  size_t matches()
    /// @returns nonzero capture index (i.e. true) if the entire input matched this matcher's pattern, zero (i.e. false) otherwise.
  {
    if (mat_ == 0 && at_bob())
      mat_ = match(Const::MATCH) && at_end();
    return mat_;
  }
  /// Returns a positive integer (true) indicating the capture index of the matched text in the pattern or zero (false) for a mismatch.
  size_t accept() const
    /// @returns nonzero capture index of the match in the pattern, which may be matcher dependent, or zero for a mismatch, or Const::EMPTY for the empty last split.
  {
    return cap_;
  }
  /// Returns pointer to the begin of the matched text (non-0-terminated), a fast constant-time operation, use with end() or use size() for text end/length.
  const char *begin() const
    /// @returns const char* pointer to the matched text in the buffer.
  {
    return txt_;
  }
  /// Returns pointer to the end of the matched text, a fast constant-time operation.
  const char *end() const
    /// @returns const char* pointer to the end of the matched text in the buffer.
  {
    return txt_ + len_;
  }
  /// Returns 0-terminated string of the text matched, a constant-time operation.
  const char *text()
    /// @returns 0-terminated const char* string with text matched.
  {
    if (chr_ == '\0')
    {
      chr_ = txt_[len_];
      const_cast<char*>(txt_)[len_] = '\0'; // cast is ugly, but OK since txt_ points in non-const buf_[]
    }
    return txt_;
  }
  /// Returns the text matched as a string, a copy of text().
  std::string str() const
    /// @returns string with text matched.
  {
    return std::string(txt_, len_);
  }
  /// Returns the match as a wide string, converted from UTF-8 text().
  std::wstring wstr() const
    /// @returns wide string with text matched.
  {
    const char *t = txt_;
    std::wstring ws;
    if (sizeof(wchar_t) == 16)
    {
      // sizeof(wchar_t) == 16: store wide string in std::wstring encoded in UTF-16
      while (t < txt_ + len_)
      {
        int wc = utf8(t, &t);
        if (wc > 0xFFFF)
        {
          if (wc <= 0x10FFFF)
          {
            ws.push_back(0xD800 | (wc - 0x010000) >> 10); // first half of UTF-16 surrogate pair
            ws.push_back(0xDC00 | (wc & 0x03FF));         // second half of UTF-16 surrogate pair
          }
          else
          {
            ws.push_back(0xFFFD);
          }
        }
        else
        {
          ws.push_back(wc);
        }
      }
    }
    else
    {
      while (t < txt_ + len_)
        ws.push_back(utf8(t, &t));
    }
    return ws;
  }
  /// Returns the length of the matched text in number of bytes, a constant-time operation.
  size_t size() const
    /// @returns match size in bytes.
  {
    return len_;
  }
  /// Returns the length of the matched text in number of wide characters.
  size_t wsize() const
    /// @returns the length of the match in number of wide (multibyte UTF-8) characters.
  {
    size_t n = 0;
    for (const char *t = txt_; t < txt_ + len_; ++t)
      n += (*t & 0xC0) != 0x80;
    return n;
  }
  /// Returns the first 8-bit character of the text matched.
  int chr() const
    /// @returns 8-bit char.
  {
    return *txt_;
  }
  /// Returns the first wide character of the text matched.
  int wchr() const
    /// @returns wide char (UTF-8 converted to Unicode).
  {
    return utf8(txt_);
  }
  /// Returns the line number of the match in the input character sequence.
  size_t lineno() const
    /// @returns line number.
  {
    size_t n = lno_;
    for (const char *s = buf_; s < txt_; ++s)
      n += (*s == '\n');
    return n;
  }
  /// Returns the column number of matched text, counting wide characters (unless compiled with WITH_BYTE_COLUMNO).
  size_t columno() const
    /// @returns column number.
  {
    for (const char *s = txt_ - 1; s >= buf_; --s)
      if (*s == '\n')
        return txt_ - s - 1;
#if defined(WITH_BYTE_COLUMNO)
    // count column offset in bytes
    return cno_ + txt_ - buf_;
#else
    // count column offset in UTF-8 chars
    size_t n = cno_;
    for (const char *t = buf_; t < txt_; ++t)
      n += (*t & 0xC0) != 0x80;
    return n;
#endif
  }
  /// Returns std::pair<size_t,std::string>(accept(), str()), useful for tokenizing input into containers of pairs.
  std::pair<size_t,std::string> pair() const
    /// @returns std::pair<size_t,std::string>(accept(), str()).
  {
    return std::pair<size_t,std::string>(accept(), str());
  }
  /// Returns std::pair<size_t,std::wstring>(accept(), wstr()), useful for tokenizing input into containers of pairs.
  std::pair<size_t,std::wstring> wpair() const
    /// @returns std::pair<size_t,std::wstring>(accept(), wstr()).
  {
    return std::pair<size_t,std::wstring>(accept(), wstr());
  }
  /// Returns the position of the first character of the match in the input character sequence, a constant-time operation.
  size_t first() const
    /// @returns position in the input character sequence.
  {
    return num_ + txt_ - buf_;
  }
  /// Returns the position of the last character + 1 of the match in the input character sequence, a constant-time operation.
  size_t last() const
    /// @returns position in the input character sequence.
  {
    return first() + size();
  }
  /// Returns true if this matcher is at the start of an input character sequence. Use reset() to restart input.
  bool at_bob() const
    /// @returns true if at the begin of an input sequence.
  {
    return got_ == Const::BOB;
  }
  /// Returns true if this matcher has no more input to read from the input character sequence.
  bool at_end()
    /// @returns true if at end of input and a read attempt will produce EOF.
  {
    return pos_ == end_ && (eof_ || peek() == EOF);
  }
  /// Returns true if this matcher hit the end of the input character sequence.
  bool hit_end() const
    /// @returns true if EOF was hit (and possibly more input would have changed the result), false otherwise (but next read attempt may return EOF immediately).
  {
    return pos_ == end_ && eof_;
  }
  /// Set and force the end of input state.
  void set_end(bool eof)
  {
    if (eof)
      flush();
    eof_ = eof;
  }
  /// Returns true if this matcher reached the begin of a new line.
  bool at_bol() const
    /// @returns true if at begin of a new line.
  {
    return got_ == '\n';
  }
  /// Set the begin of a new line state.
  void set_bol(bool bol)
  {
    if (bol)
      got_ = '\n';
    else if (got_ == '\n')
      got_ = Const::UNK;
  }
  /// Returns the next 8-bit character from the input character sequence, while preserving the current text() match (but pointer returned by text() may change; warning: does not preserve the yytext string pointer when options --flex and --bison are used).
  int input()
    /// @returns the character read (unsigned char 0..255) read or EOF (-1).
  {
    DBGLOG("AbstractMatcher::input() pos = %zu end = %zu", pos_, end_);
    if (pos_ < end_)
    {
      if (chr_ != '\0' && buf_ + pos_ == txt_ + len_)
        got_ = chr_;
      else
        got_ = static_cast<unsigned char>(buf_[pos_]);
      ++pos_;
    }
    else
    {
#if defined(WITH_FAST_GET)
      got_ = get_more();
#else
      got_ = get();
#endif
    }
    cur_ = pos_;
    return got_;
  }
  /// Returns the next wide character from the input character sequence, while preserving the current text() match (but pointer returned by text() may change; warning: does not preserve the yytext string pointer when options --flex and --bison are used).
  int winput()
    /// @returns the wide character read or EOF (-1).
  {
    DBGLOG("AbstractMatcher::winput()");
    char tmp[6], *s = tmp;
    int c;
    if ((c = input()) == EOF)
      return EOF;
    if (static_cast<unsigned char>(*s++ = c) >= 0x80)
    {
      while (((++*s = (pos_ < end_ ? buf_[pos_++] : get())) & 0xC0) == 0x80)
        continue;
      got_ = static_cast<unsigned char>(buf_[cur_ = --pos_]);
    }
    return utf8(tmp);
  }
  /// Put back one character (8-bit) on the input character sequence for matching, invalidating the current match info and text.
  void unput(char c) ///< 8-bit character to put back
  {
    DBGLOG("AbstractMatcher::unput()");
    reset_text();
    if (pos_ > 0)
    {
      --pos_;
    }
    else
    {
      txt_ = buf_;
      len_ = 0;
      if (end_ + blk_ >= max_)
        (void)grow();
      std::memmove(buf_ + 1, buf_, end_);
      ++end_;
    }
    buf_[pos_] = c;
    cur_ = pos_;
  }
  /// Fetch the rest of the input as text, useful for searching/splitting up to n times after which the rest is needed.
  const char *rest()
    /// @returns const char* string of the remaining input (wrapped with more input when AbstractMatcher::wrap is defined).
  {
    DBGLOG("AbstractMatcher::rest()");
    reset_text();
    if (pos_ > 0)
    {
      DBGLOGN("Shift buffer to close gap of %zu bytes", pos_);
      txt_ = buf_ + pos_;
      update();
      end_ -= pos_;
      std::memmove(buf_, buf_ + pos_, end_);
    }
    txt_ = buf_;
    while (!eof_)
    {
      (void)grow();
      pos_ = end_;
      end_ += get(buf_ + end_, blk_ ? blk_ : max_ - end_);
      if (pos_ == end_)
      {
        DBGLOGN("rest(): EOF");
        if (!wrap())
        {
          if (end_ == max_)
            (void)grow(1); // room for a final \0
          eof_ = true;
          break;
        }
      }
    }
    cur_ = pos_ = 0;
    len_ = end_;
    DBGLOGN("rest() length = %zu", len_);
    return text();
  }
  /// Append the next match to the currently matched text returned by AbstractMatcher::text, when the next match found is adjacent to the current match.
  void more()
  {
    cur_ = txt_ - buf_;
  }
  /// Truncate the AbstractMatcher::text length of the match to n characters in length and reposition for next match.
  void less(size_t n) ///< truncated string length
  {
    if (n < len_)
    {
      DBGCHK(pos_ < max_);
      reset_text();
      pos_ = txt_ - buf_ + n;
      DBGCHK(pos_ < max_);
      len_ = n;
      cur_ = pos_;
    }
  }
  /// Cast this matcher to positive integer indicating the nonzero capture index of the matched text in the pattern, same as AbstractMatcher::accept.
  operator size_t() const
    /// @returns nonzero capture index of a match, which may be matcher dependent, or zero for a mismatch.
  {
    return accept();
  }
  /// Cast this matcher to a std::string of the text matched by this matcher.
  operator std::string() const
    /// @returns std::string with matched text.
  {
    return str();
  }
  /// Cast this matcher to a std::wstring of the text matched by this matcher.
  operator std::wstring() const
    /// @returns std::wstring converted to UCS from the 0-terminated matched UTF-8 text.
  {
    return wstr();
  }
  /// Cast the match to std::pair<size_t,std::wstring>(accept(), wstr()), useful for tokenization into containers.
  operator std::pair<size_t,std::string>() const
    /// @returns std::pair<size_t,std::wstring>(accept(), wstr()).
  {
    return pair();
  }
  /// Returns true if matched text is equal to a string, useful for std::algorithm.
  bool operator==(const char *rhs) ///< rhs string to compare to
    /// @returns true if matched text is equal to rhs string.
    const
  {
    return std::strncmp(rhs, txt_, len_) == 0 && rhs[len_] == '\0';
  }
  /// Returns true if matched text is equalt to a string, useful for std::algorithm.
  bool operator==(const std::string& rhs) ///< rhs string to compare to
    /// @returns true if matched text is equal to rhs string.
    const
  {
    return rhs.size() == len_ && rhs.compare(0, std::string::npos, txt_, len_) == 0;
  }
  /// Returns true if capture index is equal to a given size_t value, useful for std::algorithm.
  bool operator==(size_t rhs) ///< capture index to compare accept() to
    /// @returns true if capture index is equal to rhs.
    const
  {
    return accept() == rhs;
  }
  /// Returns true if capture index is equal to a given int value, useful for std::algorithm.
  bool operator==(int rhs) ///< capture index to compare accept() to
    /// @returns true if capture index is equal to rhs.
    const
  {
    return static_cast<int>(accept()) == rhs;
  }
  /// Returns true if matched text is not equal to a string, useful for std::algorithm.
  bool operator!=(const char *rhs) ///< rhs string to compare to
    /// @returns true if matched text is not equal to rhs string.
    const
  {
    return std::strncmp(rhs, txt_, len_) != 0 || rhs[len_] != '\0'; // if static checkers complain here, they are wrong
  }
  /// Returns true if matched text is not equal to a string, useful for std::algorithm.
  bool operator!=(const std::string& rhs) ///< rhs string to compare to
    /// @returns true if matched text is not equal to rhs string.
    const
  {
    return rhs.size() > len_ || rhs.compare(0, std::string::npos, txt_, len_) != 0;
  }
  /// Returns true if capture index is not equal to a given size_t value, useful for std::algorithm.
  bool operator!=(size_t rhs) ///< capture index to compare accept() to
    /// @returns true if capture index is not equal to rhs.
    const
  {
    return accept() != rhs;
  }
  /// Returns true if capture index is not equal to a given int value, useful for std::algorithm.
  bool operator!=(int rhs) ///< capture index to compare accept() to
    /// @returns true if capture index is not equal to rhs.
    const
  {
    return static_cast<int>(accept()) != rhs;
  }
  /// Returns captured text as a std::pair<const char*,size_t> with string pointer (non-0-terminated) and length.
  virtual std::pair<const char*,size_t> operator[](size_t n)
    /// @returns std::pair of string pointer and length in the captured text, where [0] returns std::pair(begin(), size()).
    const = 0;
  Operation scan;  ///< functor to scan input (to tokenize input)
  Operation find;  ///< functor to search input
  Operation split; ///< functor to split input
  Input in;        ///< input character sequence being matched by this matcher
 protected:
  /// Construct a base abstract matcher.
  AbstractMatcher(
      const Input& input, ///< input character sequence for this matcher
      const char  *opt)   ///< option string of the form `(A|N|T(=[[:digit:]])?|;)*`
    :
      scan(this, Const::SCAN),
      find(this, Const::FIND),
      split(this, Const::SPLIT)
  {
    in = input;
    init(opt);
  }
  /// Construct a base abstract matcher.
  AbstractMatcher(
      const Input&  input, ///< input character sequence for this matcher
      const Option& opt)   ///< options
    :
      scan(this, Const::SCAN),
      find(this, Const::FIND),
      split(this, Const::SPLIT)
  {
    in = input;
    init();
    opt_ = opt;
  }
  /// Initialize the base abstract matcher at construction.
  void init(const char *opt = NULL) ///< options
  {
    DBGLOG("AbstractMatcher::init(%s)", opt ? opt : "");
#if defined(WITH_REALLOC)
    buf_ = static_cast<char*>(std::malloc(max_ = 2 * Const::BLOCK));
#else
    buf_ = new char[max_ = 2 * Const::BLOCK];
#endif
    reset(opt);
  }
  /// Returns more input (method can be overriden, as by reflex::FlexLexer::get(s, n) for example that invokes reflex::FlexLexer::LexerInput(s, n)).
  virtual size_t get(
      /// @returns the nonzero number of (less or equal to n) 8-bit characters added to buffer s from the current input, or zero when EOF.
      char  *s, ///< points to the string buffer to fill with input
      size_t n) ///< size of buffer pointed to by s
  {
    return in.get(s, n);
  }
  /// Returns true if wrapping of input after EOF is supported.
  virtual bool wrap()
    /// @returns true if input was succesfully wrapped.
  {
    return false;
  }
  /// The abstract match operation implemented by pattern matching engines derived from AbstractMatcher.
  virtual size_t match(Method method)
    /// @returns nonzero when input matched the pattern using method Const::SCAN, Const::FIND, Const::SPLIT, or Const::MATCH.
    = 0;
  /// Shift or expand the internal buffer when it is too small to accommodate more input, where the buffer size is doubled when needed.
  bool grow(size_t need = Const::BLOCK) ///< optional needed space = Const::BLOCK size by default
    /// @returns true if buffer was shifted or was enlarged
  {
    if (max_ - end_ >= need)
      return false;
    size_t gap = txt_ - buf_;
    if (gap >= need)
    {
      DBGLOGN("Shift buffer to close gap of %zu bytes", gap);
      update();
      cur_ -= gap;
      ind_ -= gap;
      pos_ -= gap;
      end_ -= gap;
      std::memmove(buf_, txt_, end_);
      txt_ = buf_;
    }
    else
    {
      size_t newmax = end_ - gap + need;
      size_t oldmax = max_;
      while (max_ < newmax)
        max_ *= 2;
      if (oldmax < max_)
      {
        DBGLOGN("Expand buffer from %zu to %zu bytes", oldmax, max_);
        update();
        cur_ -= gap;
        ind_ -= gap;
        pos_ -= gap;
        end_ -= gap;
#if defined(WITH_REALLOC)
        char *newbuf = static_cast<char*>(std::realloc(static_cast<void*>(buf_), max_));
        std::memmove(newbuf, txt_, end_);
        txt_ = buf_ = newbuf;
#else
        char *newbuf = new char[max_];
        std::memcpy(newbuf, txt_, end_);
        delete[] buf_;
        txt_ = buf_ = newbuf;
#endif
      }
    }
    return true;
  }
  /// Returns the next character read from the current input source.
  int get()
    /// @returns the character read (unsigned char 0..255) or EOF (-1).
  {
    DBGLOG("AbstractMatcher::get()");
#if defined(WITH_FAST_GET)
    return pos_ < end_ ? static_cast<unsigned char>(buf_[pos_++]) : get_more();
#else
    if (pos_ < end_)
      return static_cast<unsigned char>(buf_[pos_++]);
    if (eof_)
      return EOF;
    if (end_ + blk_ >= max_)
      (void)grow();
    while (true)
    {
      end_ += get(buf_ + end_, blk_ ? blk_ : max_ - end_);
      if (pos_ < end_)
        return static_cast<unsigned char>(buf_[pos_++]);
      DBGLOGN("get(): EOF");
      if (!wrap())
      {
        if (end_ == max_)
          (void)grow(1); // room for a final \0
        eof_ = true;
        return EOF;
      }
    }
#endif
  }
  /// Peek at the next character available for reading from the current input source.
  int peek()
    /// @returns the character (unsigned char 0..255) or EOF (-1).
  {
    DBGLOG("AbstractMatcher::peek()");
#if defined(WITH_FAST_GET)
    return pos_ < end_ ? static_cast<unsigned char>(buf_[pos_]) : peek_more();
#else
    if (pos_ < end_)
      return static_cast<unsigned char>(buf_[pos_]);
    if (eof_)
      return EOF;
    if (end_ + blk_ >= max_)
      (void)grow();
    while (true)
    {
      end_ += get(buf_ + end_, blk_ ? blk_ : max_ - end_);
      if (pos_ < end_)
        return static_cast<unsigned char>(buf_[pos_]);
      DBGLOGN("peek(): EOF");
      if (!wrap())
      {
        if (end_ == max_)
          (void)grow(1); // room for a final \0
        eof_ = true;
        return EOF;
      }
    }
#endif
  }
  /// Reset the matched text by removing the terminating \0, which is needed to search for a new match.
  void reset_text()
  {
    if (chr_ != '\0')
    {
      const_cast<char*>(txt_)[len_] = chr_;
      chr_ = '\0';
    }
  }
  /// Set the current position in the buffer for the next match.
  void set_current(size_t loc) ///< new location in buffer
  {
    DBGCHK(loc <= end_);
    pos_ = cur_ = loc;
    got_ = loc > 0 ? static_cast<unsigned char>(buf_[loc - 1]) : Const::UNK;
  }
  Option      opt_; ///< options for matcher engines
  char       *buf_; ///< input character sequence buffer
  const char *txt_; ///< points to the matched text in buffer AbstractMatcher::buf_
  size_t      len_; ///< size of the matched text
  size_t      cap_; ///< nonzero capture index of an accepted match or zero
  size_t      cur_; ///< next position in AbstractMatcher::buf_ to assign to AbstractMatcher::txt_
  size_t      pos_; ///< position in AbstractMatcher::buf_ after AbstractMatcher::txt_
  size_t      end_; ///< ending position of the input buffered in AbstractMatcher::buf_
  size_t      max_; ///< total buffer size and max position + 1 to fill
  size_t      ind_; ///< current indent position
  size_t      blk_; ///< block size for block-based input reading, as set by AbstractMatcher::buffer
  int         got_; ///< last unsigned character we looked at (to determine anchors and boundaries)
  int         chr_; ///< the character located at AbstractMatcher::txt_[AbstractMatcher::len_]
  size_t      lno_; ///< line number count (prior to this buffered input)
  size_t      cno_; ///< column number count (prior to this buffered input)
  size_t      num_; ///< character count (number of characters flushed prior to this buffered input)
  bool        eof_; ///< input has reached EOF
  bool        mat_; ///< true if AbstractMatcher::matches() was successful
 private:
#if defined(WITH_FAST_GET)
  /// Get the next character if not currently buffered.
  int get_more()
    /// @returns the character read (unsigned char 0..255) or EOF (-1).
  {
    if (eof_)
      return EOF;
    if (end_ + blk_ >= max_)
      (void)grow();
    while (true)
    {
      end_ += get(buf_ + end_, blk_ ? blk_ : max_ - end_);
      if (pos_ < end_)
        return static_cast<unsigned char>(buf_[pos_++]);
      DBGLOGN("get_more(): EOF");
      if (!wrap())
      {
        if (end_ == max_)
          (void)grow(1); // room for a final \0
        eof_ = true;
        return EOF;
      }
    }
  }
  /// Peek at the next character if not currently buffered.
  int peek_more()
    /// @returns the character (unsigned char 0..255) or EOF (-1).
  {
    if (eof_)
      return EOF;
    if (end_ + blk_ >= max_)
      (void)grow();
    while (true)
    {
      end_ += get(buf_ + end_, blk_ ? blk_ : max_ - end_);
      if (pos_ < end_)
        return static_cast<unsigned char>(buf_[pos_]);
      DBGLOGN("peek_more(): EOF");
      if (!wrap())
      {
        if (end_ == max_)
          (void)grow(1); // room for a final \0
        eof_ = true;
        return EOF;
      }
    }
  }
#endif
  /// Update the newline count, column count, and character count when shifting the buffer. 
  void update()
  {
#if defined(WITH_BYTE_COLUMNO)
    const char *t = buf_;
    for (const char *s = buf_; s < txt_; ++s)
    {
      if (*s == '\n')
      {
        ++lno_;
        t = s;
        cno_ = 0;
      }
    }
    // count column offset in bytes
    cno_ += txt_ - t;
    num_ += txt_ - buf_;
#else
    for (const char *s = buf_; s < txt_; ++s)
    {
      if (*s == '\n')
      {
        ++lno_;
        cno_ = 0;
      }
      else
      {
        // count column offset in UTF-8 chars
        cno_ += (*s & 0xC0) != 0x80;
      }
    }
    num_ += txt_ - buf_;
#endif
  }
};

/// The pattern matcher class template extends abstract matcher base class.
/** More info TODO */
template<typename P> /// @tparam <P> pattern class to instantiate a matcher
class PatternMatcher : public AbstractMatcher {
 public:
  typedef P Pattern; ///< pattern class of this matcher, a typedef of the PatternMatcher template parameter
  /// Copy constructor, the underlying pattern object is shared (not deep copied).
  PatternMatcher(const PatternMatcher& matcher) ///< matcher with pattern to use (share)
    :
      AbstractMatcher(matcher.in, matcher.opt_),
      own_(false),
      pat_(matcher.pat_)
  { }
  /// Delete matcher, deletes pattern when owned, deletes this matcher's internal buffer.
  virtual ~PatternMatcher()
  {
    DBGLOG("PatternMatcher::~PatternMatcher()");
    if (own_ && pat_ != NULL)
      delete pat_;
#if defined(WITH_REALLOC)
    std::free(static_cast<void*>(buf_));
#else
    delete[] buf_;
#endif
  }
  /// Set the pattern to use with this matcher as a shared pointer to another matcher pattern.
  virtual PatternMatcher& pattern(const PatternMatcher& matcher) ///< the other matcher
    /// @returns this matcher.
  {
    opt_ = matcher.opt_;
    return this->pattern(matcher.pattern());
  }
  /// Set the pattern to use with this matcher (the given pattern is shared and must be persistent).
  virtual PatternMatcher& pattern(const Pattern& pattern) ///< pattern object for this matcher
    /// @returns this matcher.
  {
    DBGLOG("Patternatcher::pattern()");
    if (pat_ != &pattern)
    {
      if (own_ && pat_ != NULL)
        delete pat_;
      pat_ = &pattern;
      own_ = false;
    }
    return *this;
  }
  /// Set the pattern to use with this matcher (the given pattern is shared and must be persistent).
  virtual PatternMatcher& pattern(const Pattern *pattern) ///< pattern object for this matcher
    /// @returns this matcher.
  {
    DBGLOG("Patternatcher::pattern()");
    if (pat_ != pattern)
    {
      if (own_ && pat_ != NULL)
        delete pat_;
      pat_ = pattern;
      own_ = false;
    }
    return *this;
  }
  /// Set the pattern from a regex string to use with this matcher.
  virtual PatternMatcher& pattern(const char *pattern) ///< regex string to instantiate internal pattern object
    /// @returns this matcher.
  {
    DBGLOG("Patternatcher::pattern(\"%s\")", pattern);
    if (own_ && pat_ != NULL)
      delete pat_;
    pat_ = new Pattern(pattern);
    own_ = true;
    return *this;
  }
  /// Set the pattern from a regex string to use with this matcher.
  virtual PatternMatcher& pattern(const std::string& pattern) ///< regex string to instantiate internal pattern object
    /// @returns this matcher.
  {
    DBGLOG("Patternatcher::pattern(\"%s\")", pattern.c_str());
    if (own_ && pat_ != NULL)
      delete pat_;
    pat_ = new Pattern(pattern);
    own_ = true;
    return *this;
  }
  /// Returns true if this matcher has a pattern.
  bool has_pattern() const
    /// @returns true if this matcher has a pattern.
  {
    return pat_ != NULL;
  }
  /// Returns true if this matcher has its own pattern not received from another matcher (responsible to delete).
  bool own_pattern() const
    /// @returns true if this matcher has its own pattern.
  {
    return own_ && pat_ != NULL;
  }
  /// Returns the pattern object associated with this matcher.
  const Pattern& pattern() const
    /// @returns reference to pattern object.
  {
    ASSERT(pat_ != NULL);
    return *pat_;
  }
 protected:
  /// Construct a base abstract matcher from a pointer to a persistent pattern object (that is shared with this class) and an input character sequence.
  PatternMatcher(
      const Pattern *pattern = NULL,  ///< points to pattern object for this matcher
      const Input&   input = Input(), ///< input character sequence for this matcher
      const char    *opt = NULL)      ///< option string of the form `(A|N|T(=[[:digit:]])?|;)*`
    :
      AbstractMatcher(input, opt),
      own_(false),
      pat_(pattern)
  { }
  PatternMatcher(
      const Pattern& pattern,         ///< pattern object for this matcher
      const Input&   input = Input(), ///< input character sequence for this matcher
      const char    *opt = NULL)      ///< option string of the form `(A|N|T(=[[:digit:]])?|;)*`
    :
      AbstractMatcher(input, opt),
      own_(false),
      pat_(&pattern)
  { }
  /// Construct a base abstract matcher from a regex pattern string and an input character sequence.
  PatternMatcher(
      const char  *pattern,         ///< regex string instantiates pattern object for this matcher
      const Input& input = Input(), ///< input character sequence for this matcher
      const char  *opt = NULL)      ///< option string of the form `(A|N|T(=[[:digit:]])?|;)*`
    :
      AbstractMatcher(input, opt),
      own_(true),
      pat_(new Pattern(pattern))
  { }
  /// Construct a base abstract matcher from a regex pattern string and an input character sequence.
  PatternMatcher(
      const std::string& pattern,         ///< regex string instantiates pattern object for this matcher
      const Input&       input = Input(), ///< input character sequence for this matcher
      const char        *opt = NULL)      ///< option string of the form `(A|N|T(=[[:digit:]])?|;)*`
    :
      AbstractMatcher(input, opt),
      own_(true),
      pat_(new Pattern(pattern))
  { }
  bool           own_; ///< true if PatternMatcher::pat_ was internally allocated
  const Pattern *pat_; ///< points to the pattern object used by the matcher
};

} // namespace reflex

/// Write matched text to a stream.
inline std::ostream& operator<<(std::ostream& os, const reflex::AbstractMatcher& matcher)
{
  os.write(matcher.begin(), matcher.size());
  return os;
}

/// Read stream and store all content in the matcher's buffer.
inline std::istream& operator>>(std::istream& is, reflex::AbstractMatcher& matcher)
{
  matcher.input(is).buffer();
  return is;
}

#endif
