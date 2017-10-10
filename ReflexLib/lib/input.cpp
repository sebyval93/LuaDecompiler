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
@file      input.cpp
@brief     RE/flex input character sequence class
@author    Robert van Engelen - engelen@genivia.com
@copyright (c) 2015-2017, Robert van Engelen, Genivia Inc. All rights reserved.
@copyright (c) BSD-3 License - see LICENSE.txt
*/

#include <reflex/input.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#if (defined(__WIN32__) || defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(__BORLANDC__)) && !defined(__CYGWIN__) && !defined(__MINGW32__) && !defined(__MINGW64__)
# define off_t __int64
# define ftello _ftelli64
# define fseeko _fseeki64
#else
# include <unistd.h> // off_t, fstat()
#endif

namespace reflex {

static const unsigned short codepages[][256] =
{
  // CP 437 to Unicode
  {
       0,0x263A,0x263B,0x2665,0x2666,0x2663,0x2660,0x2022,0x25D8,0x25CB,0x25D9,0x2642,0x2640,0x266A,0x266B,0x263C,
  0x25BA,0x25C4,0x2195,0x203C,0x00B6,0x00A7,0x25AC,0x21A8,0x2191,0x2193,0x2192,0x2190,0x221F,0x2194,0x25B2,0x25BC,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,   122,   123,   124,   125,   126,0x2302,
  0x00C7,0x00FC,0x00E9,0x00E2,0x00E4,0x00E0,0x00E5,0x00E7,0x00EA,0x00EB,0x00E8,0x00EF,0x00EE,0x00EC,0x00C4,0x00C5,
  0x00C9,0x00E6,0x00C6,0x00F4,0x00F6,0x00F2,0x00FB,0x00F9,0x00FF,0x00D6,0x00DC,0x00A2,0x00A3,0x00A5,0x20A7,0x0192,
  0x00E1,0x00ED,0x00F3,0x00FA,0x00F1,0x00D1,0x00AA,0x00BA,0x00BF,0x2310,0x00AC,0x00BD,0x00BC,0x00A1,0x00AB,0x00BB,
  0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,0x2555,0x2563,0x2551,0x2557,0x255D,0x255C,0x255B,0x2510,
  0x2514,0x2534,0x252C,0x251C,0x2500,0x253C,0x255E,0x255F,0x255A,0x2554,0x2569,0x2566,0x2560,0x2550,0x256C,0x2567,
  0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256B,0x256A,0x2518,0x250C,0x2588,0x2584,0x258C,0x2590,0x2580,
  0x03B1,0x00DF,0x0393,0x03C0,0x03A3,0x03C3,0x00B5,0x03C4,0x03A6,0x0398,0x03A9,0x03B4,0x221E,0x03C6,0x03B5,0x2229,
  0x2261,0x00B1,0x2265,0x2264,0x2320,0x2321,0x00F7,0x2248,0x00B0,0x2219,0x00B7,0x221A,0x207F,0x00B2,0x25A0,0x00A0
  },
  // CP 850 (updated to CP 858) to Unicode
  {
       0,     1,     2,     3,     4,     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
  0x00C7,0x00FC,0x00E9,0x00E2,0x00E4,0x00E0,0x00E5,0x00E7,0x00EA,0x00EB,0x00E8,0x00EF,0x00EE,0x00EC,0x00C4,0x00C5,
  0x00C9,0x00E6,0x00C6,0x00F4,0x00F6,0x00F2,0x00FB,0x00F9,0x00FF,0x00D6,0x00DC,0x00F8,0x00A3,0x00D8,0x00D7,0x0192,
  0x00E1,0x00ED,0x00F3,0x00FA,0x00F1,0x00D1,0x00AA,0x00BA,0x00BF,0x00AE,0x00AC,0x00BD,0x00BC,0x00A1,0x00AB,0x00BB,
  0x2591,0x2592,0x2593,0x2502,0x2524,0x00C1,0x00C2,0x00C0,0x00A9,0x2563,0x2551,0x2557,0x255D,0x00A2,0x00A5,0x2510,
  0x2514,0x2534,0x252C,0x251C,0x2500,0x253C,0x00E3,0x00C3,0x255A,0x2554,0x2569,0x2566,0x2560,0x2550,0x256C,0x00A4,
  0x00F0,0x00D0,0x00CA,0x00CB,0x00C8,0x20AC,0x00CD,0x00CE,0x00CF,0x2518,0x250C,0x2588,0x2584,0x00A6,0x00CC,0x2580,
  0x00D3,0x00DF,0x00D4,0x00D2,0x00F5,0x00D5,0x00B5,0x00FE,0x00DE,0x00DA,0x00DB,0x00D9,0x00FD,0x00DD,0x00AF,0x00B4,
  0x00AD,0x00B1,0x2017,0x00BE,0x00B6,0x00A7,0x00F7,0x00B8,0x00B0,0x00A8,0x00B7,0x00B9,0x00B3,0x00B2,0x25A0,0x00A0
  },
  // EBCDIC 0037 to ISO-8859-1
  {
       0,     1,     2,     3,   156,     9,   134,   127,   151,   141,   142,    11,    12,    13,    14,    15,
      16,    17,    18,    19,   157,   133,     8,   135,    24,    25,   146,   143,    28,    29,    30,    31,
     128,   129,   130,   131,   132,    10,    23,    27,   136,   137,   138,   139,   140,     5,     6,     7,
     144,   145,    22,   147,   148,   149,   150,     4,   152,   153,   154,   155,    20,    21,   158,    26,
      32,   160,   161,   162,   163,   164,   165,   166,   167,   168,    91,    46,    60,    40,    43,    33,
      38,   169,   170,   171,   172,   173,   174,   175,   176,   177,    93,    36,    42,    41,    59,    94,
      45,    47,   178,   179,   180,   181,   182,   183,   184,   185,   124,    44,    37,    95,    62,    63,
     186,   187,   188,   189,   190,   191,   192,   193,   194,    96,    58,    35,    64,    39,    61,    34,
     195,    97,    98,    99,   100,   101,   102,   103,   104,   105,   196,   197,   198,   199,   200,   201,
     202,   106,   107,   108,   109,   110,   111,   112,   113,   114,   203,   204,   205,   206,   207,   208,
     209,   126,   115,   116,   117,   118,   119,   120,   121,   122,   210,   211,   212,   213,   214,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   224,   225,   226,   227,   228,   229,   230,   231,
     123,    65,    66,    67,    68,    69,    70,    71,    72,    73,   232,   233,   234,   235,   236,   237,
     125,    74,    75,    76,    77,    78,    79,    80,    81,    82,   238,   239,   240,   241,   242,   243,
      92,   159,    83,    84,    85,    86,    87,    88,    89,    90,   244,   245,   246,   247,   248,   249,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,   250,   251,   252,   253,   254,   255
  },
  // CP-1250 to Unicode
  {
       0,     1,     2,     3,     4,     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
  0x20AC,   129,0x201A,   131,0x201E,0x2026,0x2020,0x2021,   136,0x2030,0x0160,0x2039,0x015A,0x0164,0x017D,0x0179,
     144,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,   152,0x2122,0x0161,0x203A,0x015B,0x0165,0x017E,0x017A,
     160,0x02C7,0x02D8,0x0141,   164,0x0104,   166,   167,   168,   169,0x015E,   171,   172,   173,   174,0x017B,
     176,   177,0x02DB,0x0142,   180,   181,   182,   183,   184,0x0105,0x015F,   187,0x013D,0x02DD,0x013E,0x017C,
  0x0154,   193,   194,0x0102,   196,0x0139,0x0106,   199,0x010C,   201,0x0118,   203,0x011A,   205,   206,0x010E,
  0x0110,0x0143,0x0147,   211,   212,0x0150,   214,   215,0x1586,0x016E,   218,0x0170,   220,   221,0x0162,   223,
  0x0155,   225,   226,0x0103,   228,0x013A,0x0107,   231,0x010D,   233,0x0119,   235,0x011B,   237,   238,0x010F,
  0x0111,0x0144,0x0148,   243,   244,0x0151,   246,   247,0x0159,0x016F,   250,0x0171,   252,   253,0x0163,0x02D9
  },
  // CP-1251 to Unicode
  {
       0,     1,     2,     3,     4,     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
  0x0402,0x0403,0x201A,0x0453,0x201E,0x2026,0x2020,0x2021,0x20AC,0x2030,0x0409,0x2039,0x040A,0x040C,0x040B,0x040F,
  0x0452,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,   152,0x2122,0x0459,0x203A,0x045A,0x045C,0x045B,0x045F,
     160,0x040E,0x045E,0x0408,   164,0x0409,   166,   167,0x0401,   169,0x0404,   171,   172,   173,   174,0x0407,
     176,   177,0x0406,0x0456,0x0491,   181,   182,   183,0x0451,0x2116,0x0454,   187,0x0458,0x0405,0x0455,0x0457,
  0x0100,0x0411,0x0412,0x0413,0x0414,0x0415,0x0416,0x0417,0x0418,0x0419,0x041A,0x041B,0x041C,0x041D,0x041E,0x041F,
  0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,0x0427,0x0428,0x0429,0x042A,0x042B,0x042C,0x042D,0x042E,0x042F,
  0x0430,0x0431,0x0432,0x0433,0x0434,0x0435,0x0436,0x0437,0x0438,0x0439,0x043A,0x043B,0x043C,0x043D,0x043E,0x043F,
  0x0440,0x0441,0x0442,0x0443,0x0444,0x0445,0x0446,0x0447,0x0448,0x0449,0x044A,0x044B,0x044C,0x044D,0x044E,0x044F
  },
  // CP-1252 to Unicode
  {
       0,     1,     2,     3,     4,     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
  0x20AC,   129,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,0x02C6,0x2030,0x0160,0x2039,0x0152,   141,0x017D,   143,
     144,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,0x02DC,0x2122,0x0161,0x203A,0x0153,   157,0x017E,0x0178,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,   202,   203,   204,   205,   206,   207,
     208,   209,   210,   211,   212,   213,   214,   215,   216,   217,   218,   219,   220,   221,   222,   223,
     224,   225,   226,   227,   228,   229,   230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   243,   244,   245,   246,   247,   248,   249,   250,   251,   252,   253,   254,   255
  },
  // CP-1253 to Unicode
  {
       0,     1,     2,     3,     4,     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
  0x20AC,   129,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,   136,0x2030,   138,0x2039,   140,   141,   142,   143,
     144,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,   152,0x2122,   154,0x203A,   156,   157,   158,   159,
     160,0x0385,0x0386,   163,   164,   165,   166,   167,   168,   169,   170,   171,   172,   173,   174,0x2015,
     176,   177,   178,   179,0x0384,   181,   182,   183,0x0388,0x0389,0x038A,   187,0x038C,   189,0x038E,0x038F,
  0x0390,0x0391,0x0392,0x0393,0x0394,0x0395,0x0396,0x0397,0x0398,0x0399,0x039A,0x039B,0x039C,0x039D,0x039E,0x039F,
  0x03A0,0x03A1,   210,0x03A3,0x03A4,0x03A5,0x03A6,0x03A7,0x03A8,0x03A9,0x03AA,0x03AB,0x03AC,0x03AD,0x03AE,0x03AF,
  0x03B0,0x03B1,0x03B2,0x03B3,0x03B4,0x03B5,0x03B6,0x03B7,0x03B8,0x03B9,0x03BA,0x03BB,0x03BC,0x03BD,0x03BE,0x03BF,
  0x03C0,0x03C1,0x03C2,0x03C3,0x03C4,0x03C5,0x03C6,0x03C7,0x03C8,0x03C9,0x03CA,0x03CB,0x03CC,0x03CD,0x03CE,0x03CF
  },
  // CP-1254 to Unicode
  {
       0,     1,     2,     3,     4,     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
  0x20AC,   129,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,0x02C6,0x2030,0x0160,0x2039,0x0152,   141,   142,   143,
     144,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,0x02DC,0x2122,0x0161,0x203A,0x0153,   157,   158,0x0178,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,   202,   203,   204,   205,   206,   207,
  0x011E,   209,   210,   211,   212,   213,   214,   215,   216,   217,   218,   219,   220,0x0130,0x015E,   223,
     224,   225,   226,   227,   228,   229,   230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
  0x011F,   241,   242,   243,   244,   245,   246,   247,   248,   249,   250,   252,   252,0x0131,0x015F,   255
  },
  // CP-1255 to Unicode
  {
       0,     1,     2,     3,     4,     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
  0x20AC,   129,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,0x02C6,0x2030,   138,0x2039,   140,   141,   142,   143,
     144,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,0x02DC,0x2122,   154,0x203A,   156,   157,   158,   159,
     160,   161,   162,   163,0x20AA,   165,   166,   167,   168,   169,0x00D7,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,0x00F7,   187,   188,   189,   190,   191,
  0x05B0,0x05B1,0x05B2,0x0583,0x0584,0x0585,0x0586,0x0587,0x0588,0x0589,0x058A,0x058B,0x058C,0x058D,0x058E,0x058F,
  0x05C0,0x05C1,0x05C2,0x05C3,0x05F0,0x05F1,0x05F2,0x05F3,0x05F4,   217,   218,   219,   220,   221,   222,   223,
  0x05D0,0x05D1,0x05D2,0x05D3,0x05D4,0x05D5,0x05D6,0x05D7,0x05D8,0x05D9,0x05DA,0x05DB,0x05DC,0x05DD,0x05DE,0x05DF,
  0x05E0,0x05E1,0x05E2,0x05E3,0x05E4,0x05E5,0x05E6,0x05E7,0x05E8,0x05E9,0x05EA,   251,   252,0x200E,0x200F,   255
  },
  // CP-1256 to Unicode
  {
       0,     1,     2,     3,     4,     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
  0x20AC,0x067E,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,0x02C6,0x2030,0x0679,0x2039,0x0152,0x0686,0x0698,0x0688,
  0x06AF,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,0x06A9,0x2122,0x0691,0x203A,0x0153,0x200C,0x200D,0x06BA,
     160,0x060C,   162,   163,   164,   165,   166,   167,   168,   169,0x06BE,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,0x061B,   187,   188,   189,   190,0x061F,
  0x06C1,0x0621,0x0622,0x0623,0x0624,0x0625,0x0626,0x0627,0x0628,0x0629,0x062A,0x062B,0x062C,0x062D,0x062E,0x062F,
  0x0630,0x0631,0x0632,0x0633,0x0634,0x0635,0x0636,0x00D7,0x0637,0x0638,0x0639,0x063A,0x0640,0x0641,0x0642,0x0643,
     224,0x0644,   226,0x0645,0x0646,0x0647,0x0648,   231,   232,   233,   234,   235,0x0649,0x064A,   238,   239,
  0x064B,0x064C,0x064D,0x064E,   244,0x064F,0x0650,   247,0x0651,   249,0x0652,   251,   252,0x200E,0x200F,0x06D2
  },
  // CP-1257 to Unicode
  {
       0,     1,     2,     3,     4,     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
  0x20AC,   129,0x201A,   131,0x201E,0x2026,0x2020,0x2021,   136,0x2030,   138,0x2039,   140,0x00A8,0x02C7,0x00B8,
     144,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,   152,0x2122,   154,0x203A,   156,0x00AF,0x02DB,   159,
     160,   161,   162,   163,   164,   165,   166,   167,0x00D8,   169,0x0156,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,0x00F8,   185,0x0157,   187,   188,   189,   190,   191,
  0x0104,0x012E,0x0100,0x0106,   196,   197,0x0118,0x0112,0x010C,   201,0x0179,0x0116,0x0122,0x0136,0x012A,0x013B,
  0x0160,0x0143,0x0145,   211,0x014C,   213,   214,   215,0x0172,0x0141,0x015A,0x016A,   220,0x017B,0x017D,   223,
  0x0105,0x012F,0x0101,0x0107,   228,   229,0x0119,0x0113,0x010D,   233,0x017A,0x0117,0x0123,0x0137,0x012B,0x013C,
  0x0161,0x0144,0x0146,   242,0x014D,   245,   246,   247,0x0173,0x0142,0x015B,0x016B,   252,0x017C,0x017E,0x02D9
  },
  // CP-1258 to Unicode
  {
       0,     1,     2,     3,     4,     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
  0x20AC,   129,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,0x02C6,0x2030,   138,0x2039,0x0152,   141,   142,   143,
     244,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,0x02DC,0x2122,   154,0x203A,0x0153,   157,   158,0x0178,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,0x0102,   196,   197,   198,   199,   200,   201,   202,   203,0x0300,   205,   206,   207,
  0x0110,   209,0x0309,   211,   212,0x01A0,   214,   215,   216,   217,   218,   219,   220,0x01AF,0x0303,   223,
     224,   225,   226,0x0103,   228,   229,   230,   231,   232,   233,   234,   235,0x0301,   237,   238,   239,
  0x0111,   241,0x0323,   243,   244,0x01A1,   246,   247,   248,   249,   250,   251,   252,0x01B0,0x20AB,   255
  },
};

void Input::file_init(void)
{
  // attempt to determine the file size with fstat()
#if !defined(HAVE_CONFIG_H) || defined(HAVE_FSTAT)
#if (defined(__WIN32__) || defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(__BORLANDC__)) && !defined(__CYGWIN__) && !defined(__MINGW32__) && !defined(__MINGW64__)
  struct _stat st;
  if (_fstat(_fileno(file_), &st) == 0 && st.st_size <= 4294967295LL)
#else
  struct stat st;
  if (::fstat(::fileno(file_), &st) == 0 && st.st_size <= 4294967295LL)
#endif
    size_ = static_cast<size_t>(st.st_size);
#endif
  // assume plain (ASCII, binary or UTF-8 without BOM) content by default
  utfx_ = file_encoding::plain;
  // check first UTF BOM byte
  if (::fread(utf8_, 1, 1, file_) == 1)
  {
    utf8_[1] = '\0';
    uidx_ = 0;
    if (utf8_[0] == '\0' || utf8_[0] == '\xef' || utf8_[0] == '\xfe' || utf8_[0] == '\xff')
    {
      // check second UTF BOM byte
      if (::fread(utf8_ + 1, 1, 1, file_) == 1)
      {
        utf8_[2] = '\0';
        if (utf8_[0] == '\0' && utf8_[1] == '\0')  // UTF-32 big endian BOM 0000XXXX?
        {
          if (::fread(utf8_ + 2, 1, 2, file_) == 2)
          {
            utf8_[4] = '\0';
            if (utf8_[2] == '\xfe' && utf8_[3] == '\xff') // UTF-32 big endian BOM 0000FEFF?
            {
              size_ = 0;
              uidx_ = sizeof(utf8_);
              utfx_ = file_encoding::utf32be;
            }
          }
        }
        else if (utf8_[0] == '\xfe' && utf8_[1] == '\xff') // UTF-16 big endian BOM FEFF?
        {
          size_ = 0;
          uidx_ = sizeof(utf8_);
          utfx_ = file_encoding::utf16be;
        }
        else if (utf8_[0] == '\xff' && utf8_[1] == '\xfe') // UTF-16 or UTF-32 little endian BOM FFFEXXXX?
        {
          if (::fread(utf8_ + 2, 1, 2, file_) == 2)
          {
            utf8_[4] = '\0';
            if (utf8_[2] == '\0' && utf8_[3] == '\0') // UTF-32 little endian BOM FFFE0000?
            {
              size_ = 0;
              uidx_ = sizeof(utf8_);
              utfx_ = file_encoding::utf32le;
            }
            else
            {
              size_ = 0;
              utf8_[utf8(utf8_[2] | utf8_[3] << 8, utf8_)] = '\0';
              uidx_ = 0;
              utfx_ = file_encoding::utf16le;
            }
          }
        }
        else if (utf8_[0] == '\xef' && utf8_[1] == '\xbb') // UTF-8 BOM EFBBXX?
        {
          if (::fread(utf8_ + 2, 1, 1, file_) == 1)
          {
            utf8_[3] = '\0';
            if (utf8_[2] == '\xbf') // UTF-8 BOM EFBBBF?
            {
              size_ -= 3;
              uidx_ = sizeof(utf8_);
              utfx_ = file_encoding::utf8;
            }
          }
        }
      }
    }
  }
}

size_t Input::file_get(char *s, size_t n)
{
  char *t = s;
  if (uidx_ < sizeof(utf8_))
  {
    unsigned short k = 0;
    while (k < n && utf8_[uidx_ + k] != '\0')
      *t++ = utf8_[uidx_ + k++];
    n -= k;
    if (n == 0)
    {
      uidx_ += k;
      return k;
    }
    uidx_ = sizeof(utf8_);
  }
  unsigned char buf[4];
  switch (utfx_)
  {
    case file_encoding::utf16be:
      while (n > 0 && ::fread(buf, 2, 1, file_) == 1)
      {
        int c = buf[0] << 8 | buf[1];
        if (c < 0x80)
        {
          *t++ = static_cast<char>(c);
          --n;
        }
        else
        {
          if (c >= 0xD800 && c < 0xE000)
          {
            // UTF-16 surrogate pair
            if (c < 0xDC00 && ::fread(buf + 2, 2, 1, file_) == 1 && (buf[2] & 0xFC) == 0xDC)
              c = 0x010000 - 0xDC00 + ((c - 0xD800) << 10) + (buf[2] << 8 | buf[3]);
            else
              c = REFLEX_NONCHAR;
          }
          size_t l = utf8(c, utf8_);
          if (n < l)
          {
            std::memcpy(t, utf8_, n);
            utf8_[l] = '\0';
            uidx_ = static_cast<unsigned short>(n);
            t += n;
            n = 0;
          }
          else
          {
            std::memcpy(t, utf8_, l);
            t += l;
            n -= l;
          }
        }
      }
      return t - s;
    case file_encoding::utf16le:
      while (n > 0 && ::fread(buf, 2, 1, file_) == 1)
      {
        int c = buf[0] | buf[1] << 8;
        if (c < 0x80)
        {
          *t++ = static_cast<char>(c);
          --n;
        }
        else
        {
          if (c >= 0xD800 && c < 0xE000)
          {
            // UTF-16 surrogate pair
            if (c < 0xDC00 && ::fread(buf + 2, 2, 1, file_) == 1 && (buf[3] & 0xFC) == 0xDC)
              c = 0x010000 - 0xDC00 + ((c - 0xD800) << 10) + (buf[2] | buf[3] << 8);
            else
              c = REFLEX_NONCHAR;
          }
          size_t l = utf8(c, utf8_);
          if (n < l)
          {
            std::memcpy(t, utf8_, n);
            utf8_[l] = '\0';
            uidx_ = static_cast<unsigned short>(n);
            t += n;
            n = 0;
          }
          else
          {
            std::memcpy(t, utf8_, l);
            t += l;
            n -= l;
          }
        }
      }
      return t - s;
    case file_encoding::utf32be:
      while (n > 0 && ::fread(buf, 4, 1, file_) == 1)
      {
        int c = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
        if (c < 0x80)
        {
          *t++ = static_cast<char>(c);
          --n;
        }
        else
        {
          size_t l = utf8(c, utf8_);
          if (n < l)
          {
            std::memcpy(t, utf8_, n);
            utf8_[l] = '\0';
            uidx_ = static_cast<unsigned short>(n);
            t += n;
            n = 0;
          }
          else
          {
            std::memcpy(t, utf8_, l);
            t += l;
            n -= l;
          }
        }
      }
      return t - s;
    case file_encoding::utf32le:
      while (n > 0 && ::fread(buf, 4, 1, file_) == 1)
      {
        int c = buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
        if (c < 0x80)
        {
          *t++ = static_cast<char>(c);
          --n;
        }
        else
        {
          size_t l = utf8(c, utf8_);
          if (n < l)
          {
            std::memcpy(t, utf8_, n);
            utf8_[l] = '\0';
            uidx_ = static_cast<unsigned short>(n);
            t += n;
            n = 0;
          }
          else
          {
            std::memcpy(t, utf8_, l);
            t += l;
            n -= l;
          }
        }
      }
      return t - s;
    case file_encoding::latin:
      while (n > 0 && ::fread(t, 1, 1, file_) == 1)
      {
        int c = static_cast<unsigned char>(*t);
        if (c < 0x80)
        {
          t++;
          --n;
        }
        else
        {
          utf8(c, utf8_);
          *t++ = utf8_[0];
          --n;
          if (n > 0)
          {
            *t++ = utf8_[1];
            --n;
          }
          else
          {
            uidx_ = 1;
            utf8_[2] = '\0';
          }
        }
      }
      return t - s;
    case file_encoding::cp437:
    case file_encoding::cp850:
    case file_encoding::ebcdic:
    case file_encoding::cp1250:
    case file_encoding::cp1251:
    case file_encoding::cp1252:
    case file_encoding::cp1253:
    case file_encoding::cp1254:
    case file_encoding::cp1255:
    case file_encoding::cp1256:
    case file_encoding::cp1257:
    case file_encoding::cp1258:
    case file_encoding::custom:
      while (n > 0 && ::fread(t, 1, 1, file_) == 1)
      {
        int c = page_[static_cast<unsigned char>(*t)];
        if (c < 0x80)
        {
          *t++ = static_cast<char>(c);
          --n;
        }
        else
        {
          size_t l = utf8(c, utf8_);
          if (n < l)
          {
            std::memcpy(t, utf8_, n);
            utf8_[l] = '\0';
            uidx_ = static_cast<unsigned short>(n);
            t += n;
            n = 0;
          }
          else
          {
            std::memcpy(t, utf8_, l);
            t += l;
            n -= l;
          }
        }
      }
      return t - s;
    default:
      return t - s + ::fread(t, 1, n, file_);
  }
}

void Input::file_size(void)
{
  if (size_ == 0)
  {
    off_t k = ftello(file_);
    if (k >= 0)
    {
      unsigned char buf[4];
      switch (utfx_)
      {
        case file_encoding::latin:
          while (::fread(buf, 1, 1, file_) == 1)
            size_ += 1 + (buf[0] >= 0x80);
          break;
        case file_encoding::cp437:
        case file_encoding::cp850:
        case file_encoding::ebcdic:
        case file_encoding::cp1250:
        case file_encoding::cp1251:
        case file_encoding::cp1252:
        case file_encoding::cp1253:
        case file_encoding::cp1254:
        case file_encoding::cp1255:
        case file_encoding::cp1256:
        case file_encoding::cp1257:
        case file_encoding::cp1258:
        case file_encoding::custom:
          while (::fread(buf, 1, 1, file_) == 1)
          {
            int c = page_[buf[0]];
            size_ += 1 + (c >= 0x80) + (c >= 0x0800) + (c >= 0x010000);
          }
          break;
        case file_encoding::utf16be:
          while (::fread(buf, 2, 1, file_) == 1)
          {
            int c = buf[0] << 8 | buf[1];
            if (c >= 0xD800 && c < 0xE000)
            {
              // UTF-16 surrogate pair
              if (c < 0xDC00 && ::fread(buf + 2, 2, 1, file_) == 1 && (buf[2] & 0xFC) == 0xDC)
                c = 0x010000 - 0xDC00 + ((c - 0xD800) << 10) + (buf[2] << 8 | buf[3]);
              else
                c = REFLEX_NONCHAR;
            }
#ifndef WITH_UTF8_UNRESTRICTED
            else if (c > 0x10FFFF)
            {
              c = REFLEX_NONCHAR;
            }
#endif
            size_ += 1 + (c >= 0x80) + (c >= 0x0800) + (c >= 0x010000) + (c >= 0x200000) + (c >= 0x04000000);
          }
          break;
        case file_encoding::utf16le:
          while (::fread(buf, 2, 1, file_) == 1)
          {
            int c = buf[0] | buf[1] << 8;
            if (c >= 0xD800 && c < 0xE000)
            {
              // UTF-16 surrogate pair
              if (c < 0xDC00 && ::fread(buf + 2, 2, 1, file_) == 1 && (buf[2] & 0xFC) == 0xDC)
                c = 0x010000 - 0xDC00 + ((c - 0xD800) << 10) + (buf[2] << 8 | buf[3]);
              else
                c = REFLEX_NONCHAR;
            }
#ifndef WITH_UTF8_UNRESTRICTED
            else if (c > 0x10FFFF)
            {
              c = REFLEX_NONCHAR;
            }
#endif
            size_ += 1 + (c >= 0x80) + (c >= 0x0800) + (c >= 0x010000) + (c >= 0x200000) + (c >= 0x04000000);
          }
          break;
        case file_encoding::utf32be:
          while (::fread(buf, 4, 1, file_) == 1)
          {
            int c = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
#ifndef WITH_UTF8_UNRESTRICTED
            if (c > 0x10FFFF)
              c = REFLEX_NONCHAR;
#endif
            size_ += 1 + (c >= 0x80) + (c >= 0x0800) + (c >= 0x010000) + (c >= 0x200000) + (c >= 0x04000000);
          }
          break;
        case file_encoding::utf32le:
          while (::fread(buf, 4, 1, file_) == 1)
          {
            int c = buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
#ifndef WITH_UTF8_UNRESTRICTED
            if (c > 0x10FFFF)
              c = REFLEX_NONCHAR;
#endif
            size_ += 1 + (c >= 0x80) + (c >= 0x0800) + (c >= 0x010000) + (c >= 0x200000) + (c >= 0x04000000);
          }
          break;
        default:
          fseeko(file_, k, SEEK_END);
          off_t n = ftello(file_);
          if (n >= k)
            size_ = static_cast<size_t>(n - k);
      }
      ::clearerr(file_);
      fseeko(file_, k, SEEK_SET);
    }
    ::clearerr(file_);
  }
}

void Input::file_encoding(unsigned short enc, const unsigned short *page)
{
  if (file_ && utfx_ != enc)
  {
    if (utfx_ == file_encoding::plain && uidx_ < sizeof(utf8_))
    {
      // translate (non-BOM) plain bytes (1 to 4 bytes) buffered in utf8_[]
      unsigned char buf[sizeof(utf8_)];
      std::memcpy(buf, utf8_, sizeof(utf8_));
      unsigned char *b = buf;
      char *t = utf8_;
      int c1, c2;
      switch (enc)
      {
        case file_encoding::latin:
          for (unsigned short i = 0; *b != '\0'; ++i)
          {
            c1 = *b++;
            if (c1 < 0x80)
              *t++ = static_cast<char>(c1);
            else
              t += utf8(c1, t);
          }
          *t = '\0';
          uidx_ = 0;
          break;
        case file_encoding::cp437:
        case file_encoding::cp850:
        case file_encoding::ebcdic:
        case file_encoding::cp1250:
        case file_encoding::cp1251:
        case file_encoding::cp1252:
        case file_encoding::cp1253:
        case file_encoding::cp1254:
        case file_encoding::cp1255:
        case file_encoding::cp1256:
        case file_encoding::cp1257:
        case file_encoding::cp1258:
          page_ = codepages[enc - file_encoding::latin - 1];
          for (unsigned short i = 0; *b != '\0'; ++i)
          {
            c1 = page_[*b++];
            if (c1 < 0x80)
              *t++ = static_cast<char>(c1);
            else
              t += utf8(c1, t);
          }
          *t = '\0';
          uidx_ = 0;
          break;
        case file_encoding::custom:
          if (page)
          {
            page_ = page;
            for (unsigned short i = 0; *b != '\0'; ++i)
            {
              c1 = page_[*b++];
              if (c1 < 0x80)
                *t++ = static_cast<char>(c1);
              else
                t += utf8(c1, t);
            }
            *t = '\0';
            uidx_ = 0;
          }
          else
          {
            enc = file_encoding::plain;
          }
          break;
        case file_encoding::utf16be:
          // enforcing non-BOM UTF-16: translate utf8_[] to UTF-16 then to UTF-8
          if (b[1] == '\0')
            ::fread(b + 1, 1, 1, file_);
          if (b[2] == '\0')
            ::fread(b + 2, 1, 2, file_);
          else if (b[3] == '\0')
            ::fread(b + 3, 1, 1, file_);
          c1 = b[0] << 8 | b[1];
          c2 = b[2] << 8 | b[3];
          if (c1 >= 0xD800 && c1 < 0xE000)
          {
            // UTF-16 surrogate pair
            if (c1 < 0xDC00 && (c2 & 0xFC00) == 0xDC00)
              c1 = 0x010000 - 0xDC00 + ((c1 - 0xD800) << 10) + c2;
            else
              c1 = REFLEX_NONCHAR;
            t += utf8(c1, t);
          }
          else
          {
            t += utf8(c1, t);
            t += utf8(c2, t);
          }
          *t = '\0';
          uidx_ = 0;
          break;
        case file_encoding::utf16le:
          // enforcing non-BOM UTF-16: translate utf8_[] to UTF-16 then to UTF-8
          if (b[1] == '\0')
            ::fread(b + 1, 1, 1, file_);
          if (b[2] == '\0')
            ::fread(b + 2, 1, 2, file_);
          else if (b[3] == '\0')
            ::fread(b + 3, 1, 1, file_);
          c1 = b[0] | b[1] << 8;
          c2 = b[2] | b[3] << 8;
          if (c1 >= 0xD800 && c1 < 0xE000)
          {
            // UTF-16 surrogate pair
            if (c1 < 0xDC00 && (c2 & 0xFC00) == 0xDC00)
              c1 = 0x010000 - 0xDC00 + ((c1 - 0xD800) << 10) + c2;
            else
              c1 = REFLEX_NONCHAR;
            t += utf8(c1, t);
          }
          else
          {
            t += utf8(c1, t);
            t += utf8(c2, t);
          }
          *t = '\0';
          uidx_ = 0;
          break;
        case file_encoding::utf32be:
          // enforcing non-BOM UTF-32: translate utf8_[] to UTF-32 then to UTF-8
          if (b[1] == '\0')
            ::fread(b + 1, 1, 3, file_);
          else if (b[2] == '\0')
            ::fread(b + 2, 1, 2, file_);
          else if (b[3] == '\0')
            ::fread(b + 3, 1, 1, file_);
          c1 = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
          t += utf8(c1, t);
          *t = '\0';
          uidx_ = 0;
          break;
        case file_encoding::utf32le:
          // enforcing non-BOM UTF-32: translate utf8_[] to UTF-32 then to UTF-8
          if (b[1] == '\0')
            ::fread(b + 1, 1, 3, file_);
          else if (b[2] == '\0')
            ::fread(b + 2, 1, 2, file_);
          else if (b[3] == '\0')
            ::fread(b + 3, 1, 1, file_);
          c1 = buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
          t += utf8(c1, t);
          *t = '\0';
          uidx_ = 0;
          break;
        default:
          break;
      }
    }
    size_ = 0;
    utfx_ = enc;
  }
}

} // namespace reflex
