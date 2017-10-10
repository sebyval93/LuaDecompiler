// lex.yy.h generated by reflex 0.9.27 from lua_format.l

#ifndef REFLEX_HEADER_H
#define REFLEX_HEADER_H
#define IN_HEADER 1

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  OPTIONS USED                                                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#define REFLEX_OPTION_fast                true
#define REFLEX_OPTION_flex                true
#define REFLEX_OPTION_header_file         lex.yy.h
#define REFLEX_OPTION_lex                 yylex
#define REFLEX_OPTION_lexer               yyFlexLexer
#define REFLEX_OPTION_outfile             lex.yy.cpp
#define REFLEX_OPTION_prefix              yy

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  REGEX MATCHER                                                             //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <reflex/matcher.h>

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  FLEX-COMPATIBLE ABSTRACT LEXER CLASS                                      //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <reflex/flexlexer.h>
typedef reflex::FlexLexer<reflex::Matcher> FlexLexer;

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  LEXER CLASS                                                               //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

class yyFlexLexer : public FlexLexer {
 public:
  yyFlexLexer(
      const reflex::Input& input = reflex::Input(),
      std::ostream        *os    = NULL)
    :
      FlexLexer(input, os)
  {
  }
  virtual int yylex();
  int yylex(
      const reflex::Input& input,
      std::ostream        *os = NULL)
  {
    in(input);
    if (os)
      out(*os);
    return yylex();
  }
};

#endif
