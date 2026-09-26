// license:BSD-3-Clause
// copyright-holders:vibecodekun
/******************************************************************************

    ISO/IEC 11172-2 (MPEG-1) video decoder

    See mpeg1video.h for the interface and for where the tables come from.

    Portions of this file are derived from PL_MPEG by Dominic Szablewski
    (https://github.com/phoboslab/pl_mpeg), used under the following
    license:

    The MIT License (MIT)

    Copyright (c) 2019 Dominic Szablewski

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to permit
    persons to whom the Software is furnished to do so, subject to the
    following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
    NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
    DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
    OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
    USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/

#include "mpeg1video.h"

#include <algorithm>
#include <cstring>

namespace {

//**************************************************************************
//  Start codes
//**************************************************************************

constexpr uint8_t START_PICTURE     = 0x00;
constexpr uint8_t START_SLICE_FIRST = 0x01;
constexpr uint8_t START_SLICE_LAST  = 0xaf;
constexpr uint8_t START_USER_DATA   = 0xb2;
constexpr uint8_t START_SEQUENCE    = 0xb3;
constexpr uint8_t START_EXTENSION   = 0xb5;
constexpr uint8_t START_SEQ_END     = 0xb7;
constexpr uint8_t START_GOP         = 0xb8;

constexpr int PICTURE_TYPE_INTRA      = 1;
constexpr int PICTURE_TYPE_PREDICTIVE = 2;
constexpr int PICTURE_TYPE_B          = 3;

//**************************************************************************
//  Tables from ISO/IEC 11172-2
//**************************************************************************

const uint8_t ZIG_ZAG[64] =
{
	 0,  1,  8, 16,  9,  2,  3, 10,
	17, 24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13,  6,  7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63
};

const uint8_t DEFAULT_INTRA_QUANT[64] =
{
	 8, 16, 19, 22, 26, 27, 29, 34,
	16, 16, 22, 24, 27, 29, 34, 37,
	19, 22, 26, 27, 29, 34, 34, 38,
	22, 22, 26, 27, 29, 34, 37, 40,
	22, 26, 27, 29, 32, 35, 40, 48,
	26, 27, 29, 32, 35, 40, 48, 58,
	26, 27, 29, 34, 38, 46, 56, 69,
	27, 29, 35, 38, 46, 56, 69, 83
};

const uint8_t DEFAULT_NON_INTRA_QUANT[64] =
{
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16
};

// Frame period in 90 kHz ticks for each sequence header frame rate code.
const uint16_t PICTURE_RATE_90KHZ[16] =
{
	0, 3754, 3750, 3600, 3003, 3000, 1800, 1502,
	1500, 0, 0, 0, 0, 0, 0, 0
};

} // anonymous namespace

//**************************************************************************
//  Variable length code trees
//**************************************************************************
//
//  Each table is a flat binary tree. Starting from node 0, one bit at a time
//  selects the left or right child. A non-zero index is the next node (already
//  multiplied by two); an index of zero means the accompanying value is the
//  result; -1 marks a code that cannot occur.

using mpeg1_detail::vlc_node;
using mpeg1_detail::vlc_node_u;

namespace {

const vlc_node MACROBLOCK_ADDRESS_INCREMENT[] =
{
	{  1 << 1,    0}, {       0,    1},  //   0: x
	{  2 << 1,    0}, {  3 << 1,    0},  //   1: 0x
	{  4 << 1,    0}, {  5 << 1,    0},  //   2: 00x
	{       0,    3}, {       0,    2},  //   3: 01x
	{  6 << 1,    0}, {  7 << 1,    0},  //   4: 000x
	{       0,    5}, {       0,    4},  //   5: 001x
	{  8 << 1,    0}, {  9 << 1,    0},  //   6: 0000x
	{       0,    7}, {       0,    6},  //   7: 0001x
	{ 10 << 1,    0}, { 11 << 1,    0},  //   8: 0000 0x
	{ 12 << 1,    0}, { 13 << 1,    0},  //   9: 0000 1x
	{ 14 << 1,    0}, { 15 << 1,    0},  //  10: 0000 00x
	{ 16 << 1,    0}, { 17 << 1,    0},  //  11: 0000 01x
	{ 18 << 1,    0}, { 19 << 1,    0},  //  12: 0000 10x
	{       0,    9}, {       0,    8},  //  13: 0000 11x
	{      -1,    0}, { 20 << 1,    0},  //  14: 0000 000x
	{      -1,    0}, { 21 << 1,    0},  //  15: 0000 001x
	{ 22 << 1,    0}, { 23 << 1,    0},  //  16: 0000 010x
	{       0,   15}, {       0,   14},  //  17: 0000 011x
	{       0,   13}, {       0,   12},  //  18: 0000 100x
	{       0,   11}, {       0,   10},  //  19: 0000 101x
	{ 24 << 1,    0}, { 25 << 1,    0},  //  20: 0000 0001x
	{ 26 << 1,    0}, { 27 << 1,    0},  //  21: 0000 0011x
	{ 28 << 1,    0}, { 29 << 1,    0},  //  22: 0000 0100x
	{ 30 << 1,    0}, { 31 << 1,    0},  //  23: 0000 0101x
	{ 32 << 1,    0}, {      -1,    0},  //  24: 0000 0001 0x
	{      -1,    0}, { 33 << 1,    0},  //  25: 0000 0001 1x
	{ 34 << 1,    0}, { 35 << 1,    0},  //  26: 0000 0011 0x
	{ 36 << 1,    0}, { 37 << 1,    0},  //  27: 0000 0011 1x
	{ 38 << 1,    0}, { 39 << 1,    0},  //  28: 0000 0100 0x
	{       0,   21}, {       0,   20},  //  29: 0000 0100 1x
	{       0,   19}, {       0,   18},  //  30: 0000 0101 0x
	{       0,   17}, {       0,   16},  //  31: 0000 0101 1x
	{       0,   35}, {      -1,    0},  //  32: 0000 0001 00x  (35 = escape)
	{      -1,    0}, {       0,   34},  //  33: 0000 0001 11x  (34 = stuffing)
	{       0,   33}, {       0,   32},  //  34: 0000 0011 00x
	{       0,   31}, {       0,   30},  //  35: 0000 0011 01x
	{       0,   29}, {       0,   28},  //  36: 0000 0011 10x
	{       0,   27}, {       0,   26},  //  37: 0000 0011 11x
	{       0,   25}, {       0,   24},  //  38: 0000 0100 00x
	{       0,   23}, {       0,   22}   //  39: 0000 0100 01x
};

// Macroblock type flags
constexpr int MB_INTRA    = 0x01;
constexpr int MB_PATTERN  = 0x02;
constexpr int MB_BACKWARD = 0x04;
constexpr int MB_FORWARD  = 0x08;
constexpr int MB_QUANT    = 0x10;

const vlc_node MACROBLOCK_TYPE_INTRA[] =
{
	{  1 << 1,    0}, {       0, 0x01},  //   0: x
	{      -1,    0}, {       0, 0x11},  //   1: 0x
};

const vlc_node MACROBLOCK_TYPE_PREDICTIVE[] =
{
	{  1 << 1,    0}, {       0, 0x0a},  //   0: x
	{  2 << 1,    0}, {       0, 0x02},  //   1: 0x
	{  3 << 1,    0}, {       0, 0x08},  //   2: 00x
	{  4 << 1,    0}, {  5 << 1,    0},  //   3: 000x
	{  6 << 1,    0}, {       0, 0x12},  //   4: 0000x
	{       0, 0x1a}, {       0, 0x01},  //   5: 0001x
	{      -1,    0}, {       0, 0x11},  //   6: 0000 0x
};

const vlc_node MACROBLOCK_TYPE_B[] =
{
	{  1 << 1,    0}, {  2 << 1,    0},  //   0: x
	{  3 << 1,    0}, {  4 << 1,    0},  //   1: 0x
	{       0, 0x0c}, {       0, 0x0e},  //   2: 1x
	{  5 << 1,    0}, {  6 << 1,    0},  //   3: 00x
	{       0, 0x04}, {       0, 0x06},  //   4: 01x
	{  7 << 1,    0}, {  8 << 1,    0},  //   5: 000x
	{       0, 0x08}, {       0, 0x0a},  //   6: 001x
	{  9 << 1,    0}, { 10 << 1,    0},  //   7: 0000x
	{       0, 0x1e}, {       0, 0x01},  //   8: 0001x
	{      -1,    0}, {       0, 0x11},  //   9: 0000 0x
	{       0, 0x16}, {       0, 0x1a},  //  10: 0000 1x
};

const vlc_node *const MACROBLOCK_TYPE[4] =
{
	nullptr,
	MACROBLOCK_TYPE_INTRA,
	MACROBLOCK_TYPE_PREDICTIVE,
	MACROBLOCK_TYPE_B
};

const vlc_node CODE_BLOCK_PATTERN[] =
{
	{  1 << 1,    0}, {  2 << 1,    0},  //   0: x
	{  3 << 1,    0}, {  4 << 1,    0},  //   1: 0x
	{  5 << 1,    0}, {  6 << 1,    0},  //   2: 1x
	{  7 << 1,    0}, {  8 << 1,    0},  //   3: 00x
	{  9 << 1,    0}, { 10 << 1,    0},  //   4: 01x
	{ 11 << 1,    0}, { 12 << 1,    0},  //   5: 10x
	{ 13 << 1,    0}, {       0,   60},  //   6: 11x
	{ 14 << 1,    0}, { 15 << 1,    0},  //   7: 000x
	{ 16 << 1,    0}, { 17 << 1,    0},  //   8: 001x
	{ 18 << 1,    0}, { 19 << 1,    0},  //   9: 010x
	{ 20 << 1,    0}, { 21 << 1,    0},  //  10: 011x
	{ 22 << 1,    0}, { 23 << 1,    0},  //  11: 100x
	{       0,   32}, {       0,   16},  //  12: 101x
	{       0,    8}, {       0,    4},  //  13: 110x
	{ 24 << 1,    0}, { 25 << 1,    0},  //  14: 0000x
	{ 26 << 1,    0}, { 27 << 1,    0},  //  15: 0001x
	{ 28 << 1,    0}, { 29 << 1,    0},  //  16: 0010x
	{ 30 << 1,    0}, { 31 << 1,    0},  //  17: 0011x
	{       0,   62}, {       0,    2},  //  18: 0100x
	{       0,   61}, {       0,    1},  //  19: 0101x
	{       0,   56}, {       0,   52},  //  20: 0110x
	{       0,   44}, {       0,   28},  //  21: 0111x
	{       0,   40}, {       0,   20},  //  22: 1000x
	{       0,   48}, {       0,   12},  //  23: 1001x
	{ 32 << 1,    0}, { 33 << 1,    0},  //  24: 0000 0x
	{ 34 << 1,    0}, { 35 << 1,    0},  //  25: 0000 1x
	{ 36 << 1,    0}, { 37 << 1,    0},  //  26: 0001 0x
	{ 38 << 1,    0}, { 39 << 1,    0},  //  27: 0001 1x
	{ 40 << 1,    0}, { 41 << 1,    0},  //  28: 0010 0x
	{ 42 << 1,    0}, { 43 << 1,    0},  //  29: 0010 1x
	{       0,   63}, {       0,    3},  //  30: 0011 0x
	{       0,   36}, {       0,   24},  //  31: 0011 1x
	{ 44 << 1,    0}, { 45 << 1,    0},  //  32: 0000 00x
	{ 46 << 1,    0}, { 47 << 1,    0},  //  33: 0000 01x
	{ 48 << 1,    0}, { 49 << 1,    0},  //  34: 0000 10x
	{ 50 << 1,    0}, { 51 << 1,    0},  //  35: 0000 11x
	{ 52 << 1,    0}, { 53 << 1,    0},  //  36: 0001 00x
	{ 54 << 1,    0}, { 55 << 1,    0},  //  37: 0001 01x
	{ 56 << 1,    0}, { 57 << 1,    0},  //  38: 0001 10x
	{ 58 << 1,    0}, { 59 << 1,    0},  //  39: 0001 11x
	{       0,   34}, {       0,   18},  //  40: 0010 00x
	{       0,   10}, {       0,    6},  //  41: 0010 01x
	{       0,   33}, {       0,   17},  //  42: 0010 10x
	{       0,    9}, {       0,    5},  //  43: 0010 11x
	{      -1,    0}, { 60 << 1,    0},  //  44: 0000 000x
	{ 61 << 1,    0}, { 62 << 1,    0},  //  45: 0000 001x
	{       0,   58}, {       0,   54},  //  46: 0000 010x
	{       0,   46}, {       0,   30},  //  47: 0000 011x
	{       0,   57}, {       0,   53},  //  48: 0000 100x
	{       0,   45}, {       0,   29},  //  49: 0000 101x
	{       0,   38}, {       0,   26},  //  50: 0000 110x
	{       0,   37}, {       0,   25},  //  51: 0000 111x
	{       0,   43}, {       0,   23},  //  52: 0001 000x
	{       0,   51}, {       0,   15},  //  53: 0001 001x
	{       0,   42}, {       0,   22},  //  54: 0001 010x
	{       0,   50}, {       0,   14},  //  55: 0001 011x
	{       0,   41}, {       0,   21},  //  56: 0001 100x
	{       0,   49}, {       0,   13},  //  57: 0001 101x
	{       0,   35}, {       0,   19},  //  58: 0001 110x
	{       0,   11}, {       0,    7},  //  59: 0001 111x
	{       0,   39}, {       0,   27},  //  60: 0000 0001x
	{       0,   59}, {       0,   55},  //  61: 0000 0010x
	{       0,   47}, {       0,   31},  //  62: 0000 0011x
};

const vlc_node MOTION[] =
{
	{  1 << 1,    0}, {       0,    0},  //   0: x
	{  2 << 1,    0}, {  3 << 1,    0},  //   1: 0x
	{  4 << 1,    0}, {  5 << 1,    0},  //   2: 00x
	{       0,    1}, {       0,   -1},  //   3: 01x
	{  6 << 1,    0}, {  7 << 1,    0},  //   4: 000x
	{       0,    2}, {       0,   -2},  //   5: 001x
	{  8 << 1,    0}, {  9 << 1,    0},  //   6: 0000x
	{       0,    3}, {       0,   -3},  //   7: 0001x
	{ 10 << 1,    0}, { 11 << 1,    0},  //   8: 0000 0x
	{ 12 << 1,    0}, { 13 << 1,    0},  //   9: 0000 1x
	{      -1,    0}, { 14 << 1,    0},  //  10: 0000 00x
	{ 15 << 1,    0}, { 16 << 1,    0},  //  11: 0000 01x
	{ 17 << 1,    0}, { 18 << 1,    0},  //  12: 0000 10x
	{       0,    4}, {       0,   -4},  //  13: 0000 11x
	{      -1,    0}, { 19 << 1,    0},  //  14: 0000 001x
	{ 20 << 1,    0}, { 21 << 1,    0},  //  15: 0000 010x
	{       0,    7}, {       0,   -7},  //  16: 0000 011x
	{       0,    6}, {       0,   -6},  //  17: 0000 100x
	{       0,    5}, {       0,   -5},  //  18: 0000 101x
	{ 22 << 1,    0}, { 23 << 1,    0},  //  19: 0000 0011x
	{ 24 << 1,    0}, { 25 << 1,    0},  //  20: 0000 0100x
	{ 26 << 1,    0}, { 27 << 1,    0},  //  21: 0000 0101x
	{ 28 << 1,    0}, { 29 << 1,    0},  //  22: 0000 0011 0x
	{ 30 << 1,    0}, { 31 << 1,    0},  //  23: 0000 0011 1x
	{ 32 << 1,    0}, { 33 << 1,    0},  //  24: 0000 0100 0x
	{       0,   10}, {       0,  -10},  //  25: 0000 0100 1x
	{       0,    9}, {       0,   -9},  //  26: 0000 0101 0x
	{       0,    8}, {       0,   -8},  //  27: 0000 0101 1x
	{       0,   16}, {       0,  -16},  //  28: 0000 0011 00x
	{       0,   15}, {       0,  -15},  //  29: 0000 0011 01x
	{       0,   14}, {       0,  -14},  //  30: 0000 0011 10x
	{       0,   13}, {       0,  -13},  //  31: 0000 0011 11x
	{       0,   12}, {       0,  -12},  //  32: 0000 0100 00x
	{       0,   11}, {       0,  -11},  //  33: 0000 0100 01x
};

const vlc_node DCT_SIZE_LUMINANCE[] =
{
	{  1 << 1,    0}, {  2 << 1,    0},  //   0: x
	{       0,    1}, {       0,    2},  //   1: 0x
	{  3 << 1,    0}, {  4 << 1,    0},  //   2: 1x
	{       0,    0}, {       0,    3},  //   3: 10x
	{       0,    4}, {  5 << 1,    0},  //   4: 11x
	{       0,    5}, {  6 << 1,    0},  //   5: 111x
	{       0,    6}, {  7 << 1,    0},  //   6: 1111x
	{       0,    7}, {  8 << 1,    0},  //   7: 1111 1x
	{       0,    8}, {      -1,    0},  //   8: 1111 11x
};

const vlc_node DCT_SIZE_CHROMINANCE[] =
{
	{  1 << 1,    0}, {  2 << 1,    0},  //   0: x
	{       0,    0}, {       0,    1},  //   1: 0x
	{       0,    2}, {  3 << 1,    0},  //   2: 1x
	{       0,    3}, {  4 << 1,    0},  //   3: 11x
	{       0,    4}, {  5 << 1,    0},  //   4: 111x
	{       0,    5}, {  6 << 1,    0},  //   5: 1111x
	{       0,    6}, {  7 << 1,    0},  //   6: 1111 1x
	{       0,    7}, {  8 << 1,    0},  //   7: 1111 11x
	{       0,    8}, {      -1,    0},  //   8: 1111 111x
};

const vlc_node *const DCT_SIZE[3] =
{
	DCT_SIZE_LUMINANCE,
	DCT_SIZE_CHROMINANCE,
	DCT_SIZE_CHROMINANCE
};

// Values are packed as 0xRRLL: run in the high byte, level in the low byte.
// 0xffff is the escape code; 0x0000 marks an impossible code.
constexpr uint16_t DCT_COEFF_ESCAPE = 0xffff;

const vlc_node_u DCT_COEFF[] =
{
	{   1 << 1,      0}, {        0, 0x0001},  //   0: x
	{   2 << 1,      0}, {   3 << 1,      0},  //   1: 0x
	{   4 << 1,      0}, {   5 << 1,      0},  //   2: 00x
	{   6 << 1,      0}, {        0, 0x0101},  //   3: 01x
	{   7 << 1,      0}, {   8 << 1,      0},  //   4: 000x
	{   9 << 1,      0}, {  10 << 1,      0},  //   5: 001x
	{        0, 0x0002}, {        0, 0x0201},  //   6: 010x
	{  11 << 1,      0}, {  12 << 1,      0},  //   7: 0000x
	{  13 << 1,      0}, {  14 << 1,      0},  //   8: 0001x
	{  15 << 1,      0}, {        0, 0x0003},  //   9: 0010x
	{        0, 0x0401}, {        0, 0x0301},  //  10: 0011x
	{  16 << 1,      0}, {        0, 0xffff},  //  11: 0000 0x
	{  17 << 1,      0}, {  18 << 1,      0},  //  12: 0000 1x
	{        0, 0x0701}, {        0, 0x0601},  //  13: 0001 0x
	{        0, 0x0102}, {        0, 0x0501},  //  14: 0001 1x
	{  19 << 1,      0}, {  20 << 1,      0},  //  15: 0010 0x
	{  21 << 1,      0}, {  22 << 1,      0},  //  16: 0000 00x
	{        0, 0x0202}, {        0, 0x0901},  //  17: 0000 10x
	{        0, 0x0004}, {        0, 0x0801},  //  18: 0000 11x
	{  23 << 1,      0}, {  24 << 1,      0},  //  19: 0010 00x
	{  25 << 1,      0}, {  26 << 1,      0},  //  20: 0010 01x
	{  27 << 1,      0}, {  28 << 1,      0},  //  21: 0000 000x
	{  29 << 1,      0}, {  30 << 1,      0},  //  22: 0000 001x
	{        0, 0x0d01}, {        0, 0x0006},  //  23: 0010 000x
	{        0, 0x0c01}, {        0, 0x0b01},  //  24: 0010 001x
	{        0, 0x0302}, {        0, 0x0103},  //  25: 0010 010x
	{        0, 0x0005}, {        0, 0x0a01},  //  26: 0010 011x
	{  31 << 1,      0}, {  32 << 1,      0},  //  27: 0000 0000x
	{  33 << 1,      0}, {  34 << 1,      0},  //  28: 0000 0001x
	{  35 << 1,      0}, {  36 << 1,      0},  //  29: 0000 0010x
	{  37 << 1,      0}, {  38 << 1,      0},  //  30: 0000 0011x
	{  39 << 1,      0}, {  40 << 1,      0},  //  31: 0000 0000 0x
	{  41 << 1,      0}, {  42 << 1,      0},  //  32: 0000 0000 1x
	{  43 << 1,      0}, {  44 << 1,      0},  //  33: 0000 0001 0x
	{  45 << 1,      0}, {  46 << 1,      0},  //  34: 0000 0001 1x
	{        0, 0x1001}, {        0, 0x0502},  //  35: 0000 0010 0x
	{        0, 0x0007}, {        0, 0x0203},  //  36: 0000 0010 1x
	{        0, 0x0104}, {        0, 0x0f01},  //  37: 0000 0011 0x
	{        0, 0x0e01}, {        0, 0x0402},  //  38: 0000 0011 1x
	{  47 << 1,      0}, {  48 << 1,      0},  //  39: 0000 0000 00x
	{  49 << 1,      0}, {  50 << 1,      0},  //  40: 0000 0000 01x
	{  51 << 1,      0}, {  52 << 1,      0},  //  41: 0000 0000 10x
	{  53 << 1,      0}, {  54 << 1,      0},  //  42: 0000 0000 11x
	{  55 << 1,      0}, {  56 << 1,      0},  //  43: 0000 0001 00x
	{  57 << 1,      0}, {  58 << 1,      0},  //  44: 0000 0001 01x
	{  59 << 1,      0}, {  60 << 1,      0},  //  45: 0000 0001 10x
	{  61 << 1,      0}, {  62 << 1,      0},  //  46: 0000 0001 11x
	{       -1,      0}, {  63 << 1,      0},  //  47: 0000 0000 000x
	{  64 << 1,      0}, {  65 << 1,      0},  //  48: 0000 0000 001x
	{  66 << 1,      0}, {  67 << 1,      0},  //  49: 0000 0000 010x
	{  68 << 1,      0}, {  69 << 1,      0},  //  50: 0000 0000 011x
	{  70 << 1,      0}, {  71 << 1,      0},  //  51: 0000 0000 100x
	{  72 << 1,      0}, {  73 << 1,      0},  //  52: 0000 0000 101x
	{  74 << 1,      0}, {  75 << 1,      0},  //  53: 0000 0000 110x
	{  76 << 1,      0}, {  77 << 1,      0},  //  54: 0000 0000 111x
	{        0, 0x000b}, {        0, 0x0802},  //  55: 0000 0001 000x
	{        0, 0x0403}, {        0, 0x000a},  //  56: 0000 0001 001x
	{        0, 0x0204}, {        0, 0x0702},  //  57: 0000 0001 010x
	{        0, 0x1501}, {        0, 0x1401},  //  58: 0000 0001 011x
	{        0, 0x0009}, {        0, 0x1301},  //  59: 0000 0001 100x
	{        0, 0x1201}, {        0, 0x0105},  //  60: 0000 0001 101x
	{        0, 0x0303}, {        0, 0x0008},  //  61: 0000 0001 110x
	{        0, 0x0602}, {        0, 0x1101},  //  62: 0000 0001 111x
	{  78 << 1,      0}, {  79 << 1,      0},  //  63: 0000 0000 0001x
	{  80 << 1,      0}, {  81 << 1,      0},  //  64: 0000 0000 0010x
	{  82 << 1,      0}, {  83 << 1,      0},  //  65: 0000 0000 0011x
	{  84 << 1,      0}, {  85 << 1,      0},  //  66: 0000 0000 0100x
	{  86 << 1,      0}, {  87 << 1,      0},  //  67: 0000 0000 0101x
	{  88 << 1,      0}, {  89 << 1,      0},  //  68: 0000 0000 0110x
	{  90 << 1,      0}, {  91 << 1,      0},  //  69: 0000 0000 0111x
	{        0, 0x0a02}, {        0, 0x0902},  //  70: 0000 0000 1000x
	{        0, 0x0503}, {        0, 0x0304},  //  71: 0000 0000 1001x
	{        0, 0x0205}, {        0, 0x0107},  //  72: 0000 0000 1010x
	{        0, 0x0106}, {        0, 0x000f},  //  73: 0000 0000 1011x
	{        0, 0x000e}, {        0, 0x000d},  //  74: 0000 0000 1100x
	{        0, 0x000c}, {        0, 0x1a01},  //  75: 0000 0000 1101x
	{        0, 0x1901}, {        0, 0x1801},  //  76: 0000 0000 1110x
	{        0, 0x1701}, {        0, 0x1601},  //  77: 0000 0000 1111x
	{  92 << 1,      0}, {  93 << 1,      0},  //  78: 0000 0000 0001 0x
	{  94 << 1,      0}, {  95 << 1,      0},  //  79: 0000 0000 0001 1x
	{  96 << 1,      0}, {  97 << 1,      0},  //  80: 0000 0000 0010 0x
	{  98 << 1,      0}, {  99 << 1,      0},  //  81: 0000 0000 0010 1x
	{ 100 << 1,      0}, { 101 << 1,      0},  //  82: 0000 0000 0011 0x
	{ 102 << 1,      0}, { 103 << 1,      0},  //  83: 0000 0000 0011 1x
	{        0, 0x001f}, {        0, 0x001e},  //  84: 0000 0000 0100 0x
	{        0, 0x001d}, {        0, 0x001c},  //  85: 0000 0000 0100 1x
	{        0, 0x001b}, {        0, 0x001a},  //  86: 0000 0000 0101 0x
	{        0, 0x0019}, {        0, 0x0018},  //  87: 0000 0000 0101 1x
	{        0, 0x0017}, {        0, 0x0016},  //  88: 0000 0000 0110 0x
	{        0, 0x0015}, {        0, 0x0014},  //  89: 0000 0000 0110 1x
	{        0, 0x0013}, {        0, 0x0012},  //  90: 0000 0000 0111 0x
	{        0, 0x0011}, {        0, 0x0010},  //  91: 0000 0000 0111 1x
	{ 104 << 1,      0}, { 105 << 1,      0},  //  92: 0000 0000 0001 00x
	{ 106 << 1,      0}, { 107 << 1,      0},  //  93: 0000 0000 0001 01x
	{ 108 << 1,      0}, { 109 << 1,      0},  //  94: 0000 0000 0001 10x
	{ 110 << 1,      0}, { 111 << 1,      0},  //  95: 0000 0000 0001 11x
	{        0, 0x0028}, {        0, 0x0027},  //  96: 0000 0000 0010 00x
	{        0, 0x0026}, {        0, 0x0025},  //  97: 0000 0000 0010 01x
	{        0, 0x0024}, {        0, 0x0023},  //  98: 0000 0000 0010 10x
	{        0, 0x0022}, {        0, 0x0021},  //  99: 0000 0000 0010 11x
	{        0, 0x0020}, {        0, 0x010e},  // 100: 0000 0000 0011 00x
	{        0, 0x010d}, {        0, 0x010c},  // 101: 0000 0000 0011 01x
	{        0, 0x010b}, {        0, 0x010a},  // 102: 0000 0000 0011 10x
	{        0, 0x0109}, {        0, 0x0108},  // 103: 0000 0000 0011 11x
	{        0, 0x0112}, {        0, 0x0111},  // 104: 0000 0000 0001 000x
	{        0, 0x0110}, {        0, 0x010f},  // 105: 0000 0000 0001 001x
	{        0, 0x0603}, {        0, 0x1002},  // 106: 0000 0000 0001 010x
	{        0, 0x0f02}, {        0, 0x0e02},  // 107: 0000 0000 0001 011x
	{        0, 0x0d02}, {        0, 0x0c02},  // 108: 0000 0000 0001 100x
	{        0, 0x0b02}, {        0, 0x1f01},  // 109: 0000 0000 0001 101x
	{        0, 0x1e01}, {        0, 0x1d01},  // 110: 0000 0000 0001 110x
	{        0, 0x1c01}, {        0, 0x1b01},  // 111: 0000 0000 0001 111x
};

//**************************************************************************
//  Inverse DCT
//**************************************************************************
//
//  Separable integer IDCT after Chen, Fralick and Smith as refined by Wang,
//  which is the formulation used by the ISO reference decoder.

constexpr int W1 = 2841;  // 2048 * sqrt(2) * cos(1 * pi / 16)
constexpr int W2 = 2676;  // 2048 * sqrt(2) * cos(2 * pi / 16)
constexpr int W3 = 2408;  // 2048 * sqrt(2) * cos(3 * pi / 16)
constexpr int W5 = 1609;  // 2048 * sqrt(2) * cos(5 * pi / 16)
constexpr int W6 = 1108;  // 2048 * sqrt(2) * cos(6 * pi / 16)
constexpr int W7 = 565;   // 2048 * sqrt(2) * cos(7 * pi / 16)

void idct_row(int *blk)
{
	int x0, x1, x2, x3, x4, x5, x6, x7, x8;

	x1 = blk[4] << 11;
	x2 = blk[6];
	x3 = blk[2];
	x4 = blk[1];
	x5 = blk[7];
	x6 = blk[5];
	x7 = blk[3];

	if (!(x1 | x2 | x3 | x4 | x5 | x6 | x7))
	{
		const int dc = blk[0] << 3;
		for (int i = 0; i < 8; i++)
			blk[i] = dc;
		return;
	}

	x0 = (blk[0] << 11) + 128;

	x8 = W7 * (x4 + x5);
	x4 = x8 + (W1 - W7) * x4;
	x5 = x8 - (W1 + W7) * x5;
	x8 = W3 * (x6 + x7);
	x6 = x8 - (W3 - W5) * x6;
	x7 = x8 - (W3 + W5) * x7;

	x8 = x0 + x1;
	x0 -= x1;
	x1 = W6 * (x3 + x2);
	x2 = x1 - (W2 + W6) * x2;
	x3 = x1 + (W2 - W6) * x3;
	x1 = x4 + x6;
	x4 -= x6;
	x6 = x5 + x7;
	x5 -= x7;

	x7 = x8 + x3;
	x8 -= x3;
	x3 = x0 + x2;
	x0 -= x2;
	x2 = (181 * (x4 + x5) + 128) >> 8;
	x4 = (181 * (x4 - x5) + 128) >> 8;

	blk[0] = (x7 + x1) >> 8;
	blk[1] = (x3 + x2) >> 8;
	blk[2] = (x0 + x4) >> 8;
	blk[3] = (x8 + x6) >> 8;
	blk[4] = (x8 - x6) >> 8;
	blk[5] = (x0 - x4) >> 8;
	blk[6] = (x3 - x2) >> 8;
	blk[7] = (x7 - x1) >> 8;
}

void idct_col(int *blk)
{
	int x0, x1, x2, x3, x4, x5, x6, x7, x8;

	x1 = blk[8 * 4] << 8;
	x2 = blk[8 * 6];
	x3 = blk[8 * 2];
	x4 = blk[8 * 1];
	x5 = blk[8 * 7];
	x6 = blk[8 * 5];
	x7 = blk[8 * 3];

	if (!(x1 | x2 | x3 | x4 | x5 | x6 | x7))
	{
		const int dc = (blk[8 * 0] + 32) >> 6;
		for (int i = 0; i < 8; i++)
			blk[8 * i] = dc;
		return;
	}

	x0 = (blk[8 * 0] << 8) + 8192;

	x8 = W7 * (x4 + x5) + 4;
	x4 = (x8 + (W1 - W7) * x4) >> 3;
	x5 = (x8 - (W1 + W7) * x5) >> 3;
	x8 = W3 * (x6 + x7) + 4;
	x6 = (x8 - (W3 - W5) * x6) >> 3;
	x7 = (x8 - (W3 + W5) * x7) >> 3;

	x8 = x0 + x1;
	x0 -= x1;
	x1 = W6 * (x3 + x2) + 4;
	x2 = (x1 - (W2 + W6) * x2) >> 3;
	x3 = (x1 + (W2 - W6) * x3) >> 3;
	x1 = x4 + x6;
	x4 -= x6;
	x6 = x5 + x7;
	x5 -= x7;

	x7 = x8 + x3;
	x8 -= x3;
	x3 = x0 + x2;
	x0 -= x2;
	x2 = (181 * (x4 + x5) + 128) >> 8;
	x4 = (181 * (x4 - x5) + 128) >> 8;

	blk[8 * 0] = (x7 + x1) >> 14;
	blk[8 * 1] = (x3 + x2) >> 14;
	blk[8 * 2] = (x0 + x4) >> 14;
	blk[8 * 3] = (x8 + x6) >> 14;
	blk[8 * 4] = (x8 - x6) >> 14;
	blk[8 * 5] = (x0 - x4) >> 14;
	blk[8 * 6] = (x3 - x2) >> 14;
	blk[8 * 7] = (x7 - x1) >> 14;
}

inline uint8_t clamp_pixel(int value)
{
	return uint8_t((value < 0) ? 0 : ((value > 255) ? 255 : value));
}

} // anonymous namespace

//**************************************************************************
//  Construction
//**************************************************************************

mpeg1_video_decoder::mpeg1_video_decoder()
{
	std::memcpy(m_intra_quant, DEFAULT_INTRA_QUANT, sizeof(m_intra_quant));
	std::memcpy(m_non_intra_quant, DEFAULT_NON_INTRA_QUANT, sizeof(m_non_intra_quant));
	std::memset(m_block_data, 0, sizeof(m_block_data));
	reset();
}

void mpeg1_video_decoder::reset()
{
	m_buffer.clear();
	m_read_pos = 0;
	m_bit_pos = 0;

	m_has_sequence = false;
	m_sequence_ended = false;
	m_width = m_height = 0;
	m_mb_width = m_mb_height = 0;
	m_luma_width = m_luma_height = 0;
	m_chroma_width = m_chroma_height = 0;
	m_frame_rate_index = 0;

	std::memcpy(m_intra_quant, DEFAULT_INTRA_QUANT, sizeof(m_intra_quant));
	std::memcpy(m_non_intra_quant, DEFAULT_NON_INTRA_QUANT, sizeof(m_non_intra_quant));

	m_picture_type = 0;
	m_temporal_reference = 0;
	m_gop_timecode = 0;
	m_slice_begin = false;
	m_display_frame = nullptr;

	for (frame &f : m_frames)
		f = frame();

	m_work = &m_frames[0];
	m_past = &m_frames[1];
	m_future = &m_frames[2];
}

void mpeg1_video_decoder::clear_fifo()
{
	sequence_state sequence;
	get_sequence_state(sequence);
	reset();
	set_sequence_state(sequence);
}

uint16_t mpeg1_video_decoder::frame_period_90khz() const
{
	return PICTURE_RATE_90KHZ[m_frame_rate_index & 0x0f];
}

void mpeg1_video_decoder::get_sequence_state(sequence_state &state) const
{
	state.valid = m_has_sequence ? 1 : 0;
	state.width = uint16_t(m_width);
	state.height = uint16_t(m_height);
	state.frame_rate_index = m_frame_rate_index;
	std::memcpy(state.intra_quant, m_intra_quant, sizeof(state.intra_quant));
	std::memcpy(state.non_intra_quant, m_non_intra_quant, sizeof(state.non_intra_quant));
}

void mpeg1_video_decoder::set_sequence_state(const sequence_state &state)
{
	if (!state.valid || !state.width || !state.height)
		return;

	m_frame_rate_index = state.frame_rate_index;
	std::memcpy(m_intra_quant, state.intra_quant, sizeof(m_intra_quant));
	std::memcpy(m_non_intra_quant, state.non_intra_quant, sizeof(m_non_intra_quant));

	if (int(state.width) != m_width || int(state.height) != m_height)
	{
		m_width = state.width;
		m_height = state.height;
		m_mb_width = (m_width + 15) / 16;
		m_mb_height = (m_height + 15) / 16;
		m_luma_width = m_mb_width * 16;
		m_luma_height = m_mb_height * 16;
		m_chroma_width = m_luma_width / 2;
		m_chroma_height = m_luma_height / 2;
		allocate_frames();
	}

	// The reference pictures are gone, so the first few frames after this will
	// be wrong until the next intra picture arrives. That is the same thing
	// hardware does when you seek.
	m_has_sequence = true;
}

void mpeg1_video_decoder::get_decoder_state(decoder_state &state) const
{
	// Rebase the input buffer so the read position becomes byte 0. Nothing
	// before it is ever looked at again, and dropping it is what keeps the
	// saved copy inside the buffer limit even when compaction is overdue.
	const size_t size = m_buffer.size() - m_read_pos;
	state.buffer_size = uint32_t(size);
	state.bit_pos = uint32_t(m_bit_pos - m_read_pos * 8);
	if (size != 0)
		std::memcpy(state.buffer, m_buffer.data() + m_read_pos, size);
	// Unused bytes are serialized too. Do not retain data from an earlier
	// save: speculative/replayed frames can otherwise change the state file
	// even when the live decoder, audio and picture are identical.
	std::memset(state.buffer + size, 0, decoder_state::max_buffer - size);

	state.sequence_ended = m_sequence_ended ? 1 : 0;
	state.picture_type = m_picture_type | (m_gop_timecode << 3);
	state.temporal_reference = m_temporal_reference;

	const auto index_of = [this](const frame *f) -> int32_t
	{
		return (f == nullptr) ? -1 : int32_t(f - m_frames);
	};

	state.past_index = index_of(m_past);
	state.future_index = index_of(m_future);
	state.work_index = index_of(m_work);
	state.display_index = index_of(m_display_frame);

	const size_t luma = size_t(m_luma_width) * m_luma_height;
	const size_t chroma = size_t(m_chroma_width) * m_chroma_height;
	state.frames_saved = (luma != 0 && luma <= decoder_state::max_luma && chroma <= decoder_state::max_chroma) ? 1 : 0;

	for (int i = 0; i < 3; i++)
	{
		state.frame_valid[i] = m_frames[i].valid ? 1 : 0;
		state.frame_picture_type[i] = m_frames[i].picture_type | (m_frames[i].timecode << 3);
		state.frame_temporal_reference[i] = m_frames[i].temporal_reference;

		if (state.frames_saved == 0 || m_frames[i].y.size() != luma)
			continue;

		std::memcpy(state.frame_y[i], m_frames[i].y.data(), luma);
		std::memcpy(state.frame_cb[i], m_frames[i].cb.data(), chroma);
		std::memcpy(state.frame_cr[i], m_frames[i].cr.data(), chroma);
	}
}

void mpeg1_video_decoder::set_decoder_state(const decoder_state &state)
{
	const size_t size = std::min<size_t>(state.buffer_size, decoder_state::max_buffer);
	m_buffer.assign(state.buffer, state.buffer + size);
	m_read_pos = 0;
	m_bit_pos = std::min<size_t>(state.bit_pos, size * 8);

	m_sequence_ended = state.sequence_ended != 0;
	m_picture_type = state.picture_type & 7;
	m_gop_timecode = uint32_t(state.picture_type) >> 3;
	m_temporal_reference = state.temporal_reference;

	const auto frame_at = [this](int32_t index) -> frame *
	{
		return (index >= 0 && index < 3) ? &m_frames[index] : nullptr;
	};

	// The three working pictures are never null in a state written by
	// get_decoder_state(); fall back to the reset assignment rather than
	// dereference a null if one ever arrives from elsewhere.
	m_work = frame_at(state.work_index);
	m_past = frame_at(state.past_index);
	m_future = frame_at(state.future_index);
	m_display_frame = frame_at(state.display_index);

	if (m_work == nullptr)
		m_work = &m_frames[0];
	if (m_past == nullptr)
		m_past = &m_frames[1];
	if (m_future == nullptr)
		m_future = &m_frames[2];

	for (int i = 0; i < 3; i++)
	{
		m_frames[i].picture_type = state.frame_picture_type[i] & 7;
		m_frames[i].timecode = uint32_t(state.frame_picture_type[i]) >> 3;
		m_frames[i].temporal_reference = state.frame_temporal_reference[i];
		m_frames[i].valid = state.frame_valid[i] != 0;
	}

	// Without usable reference pictures, say so rather than predict from
	// whatever the freshly allocated buffers hold. The stream then picks up
	// again at the next intra picture.
	const size_t luma = size_t(m_luma_width) * m_luma_height;
	const size_t chroma = size_t(m_chroma_width) * m_chroma_height;
	const bool restorable = state.frames_saved != 0 && luma != 0
		&& luma <= decoder_state::max_luma && chroma <= decoder_state::max_chroma;

	for (int i = 0; i < 3; i++)
	{
		if (restorable && m_frames[i].y.size() == luma)
		{
			std::memcpy(m_frames[i].y.data(), state.frame_y[i], luma);
			std::memcpy(m_frames[i].cb.data(), state.frame_cb[i], chroma);
			std::memcpy(m_frames[i].cr.data(), state.frame_cr[i], chroma);
		}
		else
		{
			m_frames[i].valid = false;
		}
	}

	if (m_display_frame != nullptr && !m_display_frame->valid)
		m_display_frame = nullptr;
}

//**************************************************************************
//  Input buffer
//**************************************************************************

void mpeg1_video_decoder::write(const uint8_t *data, size_t length)
{
	// Reclaim whatever has already been decoded before deciding there is no
	// room. Only bytes ahead of the read position are still needed, so compact
	// as soon as the raw vector would run past the limit; that keeps the test
	// below a test of how much undecoded data there is, which is the same thing
	// buffer_full() reports.
	if (m_read_pos && m_buffer.size() + length > m_buffer_limit)
	{
		m_buffer.erase(m_buffer.begin(), m_buffer.begin() + m_read_pos);
		m_bit_pos -= m_read_pos * 8;
		m_read_pos = 0;
	}

	if (m_buffer.size() + length > m_buffer_limit)
	{
		// The cartridge's MPEG buffer is finite, and a full one stops accepting
		// data. Reaching here means the driver kept sending after SYS_STS told
		// it to stop, since buffer_full() goes true with room still to spare.
		// Dropping the oldest undecoded bytes instead would tear a hole out of
		// the middle of a picture and desynchronise the bit reader, which shows
		// up as video skipping ahead for no visible reason.
		return;
	}

	m_buffer.insert(m_buffer.end(), data, data + length);
}

bool mpeg1_video_decoder::have_bits(size_t count) const
{
	return m_bit_pos + count <= m_buffer.size() * 8;
}

uint32_t mpeg1_video_decoder::read_bits(int count)
{
	uint32_t value = 0;
	while (count--)
		value = (value << 1) | uint32_t(read_bit());
	return value;
}

int mpeg1_video_decoder::read_bit()
{
	if (m_bit_pos >= m_buffer.size() * 8)
	{
		m_bit_pos++;
		return 0;
	}

	const int bit = (m_buffer[m_bit_pos >> 3] >> (7 - (m_bit_pos & 7))) & 1;
	m_bit_pos++;
	return bit;
}

void mpeg1_video_decoder::align_to_byte()
{
	m_bit_pos = (m_bit_pos + 7) & ~size_t(7);
}

int mpeg1_video_decoder::read_vlc(const vlc_node *table)
{
	vlc_node state = { 0, 0 };
	do
	{
		state = table[state.index + read_bit()];
	} while (state.index > 0);

	return state.value;
}

uint16_t mpeg1_video_decoder::read_vlc_uint(const vlc_node_u *table)
{
	vlc_node_u state = { 0, 0 };
	do
	{
		state = table[state.index + read_bit()];
	} while (state.index > 0);

	return state.value;
}

size_t mpeg1_video_decoder::find_start_code(size_t from) const
{
	if (m_buffer.size() < 4)
		return SIZE_MAX;

	for (size_t i = from; i + 3 < m_buffer.size(); i++)
	{
		if (m_buffer[i] == 0x00 && m_buffer[i + 1] == 0x00 && m_buffer[i + 2] == 0x01)
			return i;
	}

	return SIZE_MAX;
}

bool mpeg1_video_decoder::at_start_code() const
{
	// Slices are padded with zero bits to a byte boundary, so a start code is
	// indicated by the next 23 bits being zero.
	size_t pos = m_bit_pos;
	for (int i = 0; i < 23; i++, pos++)
	{
		if (pos >= m_buffer.size() * 8)
			return true;
		if ((m_buffer[pos >> 3] >> (7 - (pos & 7))) & 1)
			return false;
	}
	return true;
}

void mpeg1_video_decoder::discard_consumed()
{
	m_read_pos = m_bit_pos >> 3;

	// Keep the buffer from growing without bound by dropping fully consumed
	// bytes once enough have accumulated.
	if (m_read_pos >= 65536)
	{
		m_buffer.erase(m_buffer.begin(), m_buffer.begin() + m_read_pos);
		m_bit_pos -= m_read_pos * 8;
		m_read_pos = 0;
	}
}

int mpeg1_video_decoder::pictures_buffered() const
{
	int count = 0;
	for (size_t i = m_read_pos; i + 3 < m_buffer.size(); i++)
	{
		if (m_buffer[i] == 0x00 && m_buffer[i + 1] == 0x00 && m_buffer[i + 2] == 0x01 && m_buffer[i + 3] == START_PICTURE)
			count++;
	}
	return count;
}

//**************************************************************************
//  Headers
//**************************************************************************

bool mpeg1_video_decoder::decode_sequence_header()
{
	// horizontal 12, vertical 12, aspect 4, frame rate 4, bit rate 18,
	// marker 1, vbv 10, constrained 1, then the two optional matrices.
	if (!have_bits(64))
		return false;

	const int width = int(read_bits(12));
	const int height = int(read_bits(12));
	read_bits(4);                                   // aspect ratio
	const uint8_t frame_rate_index = uint8_t(read_bits(4));
	read_bits(18);                                  // bit rate
	read_bits(1);                                   // marker
	read_bits(10);                                  // vbv buffer size
	read_bits(1);                                   // constrained parameters

	// DMA can stop anywhere in either optional matrix. Do not publish a
	// partial header or read beyond the input: decode() rewinds to the start
	// code and retries when more bytes arrive. Otherwise discard_consumed()
	// can put m_read_pos past the buffer and the next save underflows its size.
	uint8_t intra_quant[64];
	uint8_t non_intra_quant[64];
	std::memcpy(intra_quant, m_has_sequence ? m_intra_quant : DEFAULT_INTRA_QUANT, sizeof(intra_quant));
	std::memcpy(non_intra_quant, m_has_sequence ? m_non_intra_quant : DEFAULT_NON_INTRA_QUANT, sizeof(non_intra_quant));

	if (read_bit())
	{
		if (!have_bits(64 * 8 + 1)) // matrix plus the non-intra matrix flag
			return false;
		for (int i = 0; i < 64; i++)
			intra_quant[ZIG_ZAG[i]] = uint8_t(read_bits(8));
	}

	if (read_bit())
	{
		if (!have_bits(64 * 8))
			return false;
		for (int i = 0; i < 64; i++)
			non_intra_quant[ZIG_ZAG[i]] = uint8_t(read_bits(8));
	}

	m_frame_rate_index = frame_rate_index;
	std::memcpy(m_intra_quant, intra_quant, sizeof(m_intra_quant));
	std::memcpy(m_non_intra_quant, non_intra_quant, sizeof(m_non_intra_quant));

	if (width != m_width || height != m_height)
	{
		m_width = width;
		m_height = height;
		m_mb_width = (width + 15) / 16;
		m_mb_height = (height + 15) / 16;
		m_luma_width = m_mb_width * 16;
		m_luma_height = m_mb_height * 16;
		m_chroma_width = m_luma_width / 2;
		m_chroma_height = m_luma_height / 2;
		allocate_frames();
	}

	m_has_sequence = true;
	return true;
}

void mpeg1_video_decoder::allocate_frames()
{
	for (frame &f : m_frames)
	{
		f.y.assign(size_t(m_luma_width) * m_luma_height, 0);
		f.cb.assign(size_t(m_chroma_width) * m_chroma_height, 128);
		f.cr.assign(size_t(m_chroma_width) * m_chroma_height, 128);
		f.valid = false;
	}
	m_display_frame = nullptr;
}

void mpeg1_video_decoder::decode_gop_header()
{
	read_bits(1); // drop-frame flag
	const uint32_t hours_minutes = read_bits(11);
	read_bits(1); // marker
	const uint32_t seconds_pictures = read_bits(12);
	m_gop_timecode = (seconds_pictures << 16) | hours_minutes;
	read_bits(1);   // closed gop
	read_bits(1);   // broken link
}

//**************************************************************************
//  Top level
//**************************************************************************

bool mpeg1_video_decoder::decode()
{
	for (;;)
	{
		const size_t code_pos = find_start_code(m_bit_pos >> 3);
		if (code_pos == SIZE_MAX)
		{
			discard_consumed();
			return false;
		}

		const uint8_t code = m_buffer[code_pos + 3];
		m_bit_pos = (code_pos + 4) * 8;

		if (code == START_SEQUENCE)
		{
			if (!decode_sequence_header())
			{
				m_bit_pos = code_pos * 8;
				return false;
			}
		}
		else if (code == START_GOP)
		{
			if (!have_bits(27))
			{
				m_bit_pos = code_pos * 8;
				return false;
			}
			decode_gop_header();
		}
		else if (code == START_SEQ_END)
		{
			m_sequence_ended = true;
		}
		else if (code == START_USER_DATA || code == START_EXTENSION)
		{
			// Skipped; the following start code search steps over the payload.
		}
		else if (code == START_PICTURE)
		{
			if (!m_has_sequence)
				continue;

			// A picture can only be decoded once all of it is buffered, which
			// is the case when another start code that ends it has arrived.
			size_t end = code_pos + 4;
			for (;;)
			{
				end = find_start_code(end);
				if (end == SIZE_MAX)
					break;

				const uint8_t next = m_buffer[end + 3];
				if (next == START_PICTURE || next == START_GOP || next == START_SEQUENCE || next == START_SEQ_END)
					break;

				end += 4;
			}

			if (end == SIZE_MAX)
			{
				// Rewind to the picture start code and wait for more data.
				m_bit_pos = code_pos * 8;
				return false;
			}

			if (!decode_picture())
			{
				m_bit_pos = code_pos * 8;
				return false;
			}

			m_bit_pos = end * 8;
			discard_consumed();
			return true;
		}
	}
}

void mpeg1_video_decoder::display_decoded_frame()
{
	// decode_picture() rotates I/P pictures into m_future; B pictures stay
	// in m_work.  Keep those reference buffers intact for later decoding.
	frame *const decoded = (m_picture_type == PICTURE_TYPE_B) ? m_work : m_future;
	m_display_frame = decoded && decoded->valid ? decoded : nullptr;
}

bool mpeg1_video_decoder::decode_picture()
{
	if (!have_bits(29))
		return false;

	m_temporal_reference = int(read_bits(10));
	m_picture_type = int(read_bits(3));
	read_bits(16);  // vbv delay

	if (m_picture_type != PICTURE_TYPE_INTRA &&
		m_picture_type != PICTURE_TYPE_PREDICTIVE &&
		m_picture_type != PICTURE_TYPE_B)
	{
		return false;
	}

	if (m_picture_type == PICTURE_TYPE_PREDICTIVE || m_picture_type == PICTURE_TYPE_B)
	{
		m_full_pel_forward = read_bit() != 0;
		m_forward_f_code = int(read_bits(3));
		if (m_forward_f_code == 0)
			return false;
		m_forward_r_size = m_forward_f_code - 1;
		m_forward_f = 1 << m_forward_r_size;
	}

	if (m_picture_type == PICTURE_TYPE_B)
	{
		m_full_pel_backward = read_bit() != 0;
		m_backward_f_code = int(read_bits(3));
		if (m_backward_f_code == 0)
			return false;
		m_backward_r_size = m_backward_f_code - 1;
		m_backward_f = 1 << m_backward_r_size;
	}

	while (read_bit())          // extra_bit_picture
		read_bits(8);

	// Nothing has been written to this picture yet. The address has to start
	// from a known point so that the gap check below can tell which macroblocks
	// the slices have actually covered.
	m_macroblock_address = -1;
	// A seek can enter an open GOP: the I picture is self-contained, but
	// leading B pictures may still reference the anchor before the seek.
	// Prediction invalidates this picture if any required reference is absent.
	// Invalid P anchors must also stay invalid until an I picture recovers.
	m_work->valid = true;

	int last_vertical_position = 0;
	bool slice_failed = false;

	// Decode every slice belonging to this picture.
	for (;;)
	{
		const size_t code_pos = find_start_code(m_bit_pos >> 3);
		if (code_pos == SIZE_MAX)
			break;

		const uint8_t code = m_buffer[code_pos + 3];
		if (code < START_SLICE_FIRST || code > START_SLICE_LAST)
			break;

		// Slices run down the picture in order, so a vertical position that
		// goes backwards belongs to no slice at all: it is a byte-aligned
		// 00 00 01 pattern sitting in damaged slice data, which the rule against
		// start code emulation only excludes for macroblocks that were validly
		// coded to begin with. Decoding one puts the address back at the top of
		// the picture and overwrites the rows that came out fine - one damaged
		// slice cost 172 of this stream's 216 macroblocks that way. Once a slice
		// has failed the test is stricter, because any pattern found next is
		// being read out of the wreckage of the slice that just failed.
		if (int(code) < last_vertical_position ||
			(slice_failed && int(code) <= last_vertical_position))
			break;
		last_vertical_position = int(code);

		// Whatever the slices so far have left uncovered - because one of them
		// was abandoned part way through - is filled in before this slice moves
		// the address to its own row. A slice that merely starts late in its
		// row leaves no gap, because the slice before it ran on past the end of
		// its own row and already covered those macroblocks.
		conceal_through((int(code) - 1) * m_mb_width - 1);

		m_bit_pos = (code_pos + 4) * 8;
		decode_slice(code);
		slice_failed = m_slice_error;

		// Once the last macroblock has been written the picture is finished. A
		// well-formed stream has nothing after it, but a damaged one can leave
		// what looks like another slice header lying around, and decoding it
		// would re-run rows that are already correct - from a slice position
		// that no longer matches, so they come out jumbled.
		if (m_macroblock_address >= m_mb_width * m_mb_height - 1)
			break;
	}

	// And the same for the tail of the picture, as pl_mpeg does. On a clean
	// stream the slices cover everything and neither call does anything. When a
	// slice dies part way through, this is what keeps the damage invisible:
	// without it the missing macroblocks keep whatever the recycled frame
	// buffer held - a picture several frames old - and because the result is
	// then used as a reference, the wreckage is predicted forward until the
	// next intra picture.
	conceal_through(m_mb_width * m_mb_height - 1);

	m_work->picture_type = m_picture_type;
	m_work->temporal_reference = m_temporal_reference;
	m_work->timecode = m_gop_timecode;

	if (m_picture_type == PICTURE_TYPE_B)
	{
		// B pictures are never used as a reference, so they are shown as soon
		// as they are decoded.
		m_display_frame = m_work->valid ? m_work : nullptr;
	}
	else
	{
		// A new anchor has arrived, so the anchor it follows has reached its
		// display time. Rotate: the old future anchor becomes the past one,
		// the picture just decoded becomes the future anchor, and the buffer
		// the old past anchor used is recycled for the next picture.
		frame *const displaced = m_future;
		frame *const recycled = m_past;

		m_past = displaced;
		m_future = m_work;
		m_work = recycled;

		m_display_frame = displaced->valid ? displaced : nullptr;
	}

	return true;
}

//**************************************************************************
//  Slice and macroblock layer
//**************************************************************************

void mpeg1_video_decoder::decode_slice(int vertical_position)
{
	m_quantizer_scale = int(read_bits(5));

	while (read_bit())          // extra_bit_slice
		read_bits(8);

	m_macroblock_address = (vertical_position - 1) * m_mb_width - 1;
	m_slice_begin = true;
	m_slice_error = false;

	m_dc_predictor[0] = m_dc_predictor[1] = m_dc_predictor[2] = 128;
	m_motion_fw_h = m_motion_fw_v = 0;
	m_motion_bw_h = m_motion_bw_v = 0;

	do
	{
		decode_macroblock();
	} while (!m_slice_error && !at_start_code() && m_macroblock_address < m_mb_width * m_mb_height - 1);

}

void mpeg1_video_decoder::conceal_through(int last)
{
	const int limit = m_mb_width * m_mb_height;
	if (last >= limit)
		last = limit - 1;

	if (m_macroblock_address >= last)
		return;

	// Concealment is a copy of the co-located macroblock with no motion. A B
	// picture keeps whichever directions the last decoded macroblock used, so
	// that a gap does not visibly brighten or darken against its neighbours;
	// anything else predicts forwards, which for an intra picture means the
	// previous anchor.
	m_macroblock_intra = false;
	m_motion_fw_h = m_motion_fw_v = 0;
	m_motion_bw_h = m_motion_bw_v = 0;
	if (m_picture_type != PICTURE_TYPE_B || !(m_has_forward || m_has_backward))
	{
		m_has_forward = true;
		m_has_backward = false;
	}

	while (m_macroblock_address < last)
	{
		m_macroblock_address++;
		m_mb_row = m_macroblock_address / m_mb_width;
		m_mb_col = m_macroblock_address % m_mb_width;
		predict_macroblock();
	}
}

void mpeg1_video_decoder::decode_macroblock()
{
	int increment = 0;
	for (;;)
	{
		const int t = read_vlc(MACROBLOCK_ADDRESS_INCREMENT);
		if (t == 34)            // stuffing
			continue;
		if (t == 35)            // escape
		{
			increment += 33;
			continue;
		}
		increment += t;
		break;
	}

	// Every macroblock advances the address by at least one, so a zero
	// increment means the bit reader is no longer on a macroblock boundary.
	if (increment == 0)
	{
		m_slice_error = true;
		return;
	}

	if (m_slice_begin)
	{
		// The first macroblock of a slice counts its increment from the slice's
		// own vertical position, not from the macroblock before it, so the
		// macroblocks it steps over are not skipped ones - they belong to the
		// preceding slice, which may well have already decoded them. Slices are
		// free to run past the end of a row, and Monty Python's streams use
		// that constantly: a slice announced at row 2 can start five
		// macroblocks into it because the row-1 slice spilled over. Treating
		// the step as a run of skipped macroblocks re-predicted those five and
		// destroyed what the previous slice had correctly decoded, which is
		// what showed up on screen as jigsawed blocks. Only advance.
		m_slice_begin = false;
		m_macroblock_address += increment;
	}
	else
	{
		// An increment that would carry the address past the last macroblock
		// cannot be right, so the macroblock is dropped without advancing.
		// Taking it anyway leaves the address one past the end, which ends the
		// slice a macroblock short and silently drops the rest of its row.
		if (m_macroblock_address + increment >= m_mb_width * m_mb_height)
		{
			m_slice_error = true;
			return;
		}

		if (increment > 1)
		{
			// Skipped macroblocks. Intra DC prediction restarts either way. In
			// a P picture a skipped macroblock is a straight copy with no
			// motion; in a B picture it repeats the previous macroblock's
			// vectors and prediction mode.
			m_dc_predictor[0] = m_dc_predictor[1] = m_dc_predictor[2] = 128;

			if (m_picture_type != PICTURE_TYPE_B)
			{
				m_motion_fw_h = m_motion_fw_v = 0;
				m_has_forward = true;
				m_has_backward = false;
			}

			m_macroblock_intra = false;

			// An intra picture has no skipped macroblocks, so reaching here at
			// all means the stream is damaged. Predicting them from the
			// previous anchor conceals the gap; leaving them, as this used to,
			// exposes the recycled frame buffer.
			for (int i = 1; i < increment; i++)
			{
				m_macroblock_address++;
				m_mb_row = m_macroblock_address / m_mb_width;
				m_mb_col = m_macroblock_address % m_mb_width;
				predict_macroblock();
			}
		}

		m_macroblock_address++;
	}

	if (m_macroblock_address >= m_mb_width * m_mb_height)
		return;

	m_mb_row = m_macroblock_address / m_mb_width;
	m_mb_col = m_macroblock_address % m_mb_width;

	m_macroblock_type = read_vlc(MACROBLOCK_TYPE[m_picture_type]);
	if (m_macroblock_type == 0)
	{
		// No code in the table for this picture type produces zero, so the bit
		// reader has drifted. Give the macroblock back to be concealed rather
		// than decoding blocks from the wrong offset.
		m_slice_error = true;
		m_macroblock_address--;
		return;
	}

	m_macroblock_intra = (m_macroblock_type & MB_INTRA) != 0;
	m_has_forward = (m_macroblock_type & MB_FORWARD) != 0;
	m_has_backward = (m_macroblock_type & MB_BACKWARD) != 0;

	if (m_macroblock_type & MB_QUANT)
		m_quantizer_scale = int(read_bits(5));

	if (m_macroblock_intra)
	{
		m_motion_fw_h = m_motion_fw_v = 0;
		m_motion_bw_h = m_motion_bw_v = 0;
	}
	else
	{
		m_dc_predictor[0] = m_dc_predictor[1] = m_dc_predictor[2] = 128;

		if (m_has_forward)
		{
			decode_motion_vector(m_motion_fw_h, read_vlc(MOTION), m_forward_r_size, m_forward_f);
			decode_motion_vector(m_motion_fw_v, read_vlc(MOTION), m_forward_r_size, m_forward_f);
		}
		else if (m_picture_type == PICTURE_TYPE_PREDICTIVE)
		{
			m_motion_fw_h = m_motion_fw_v = 0;
		}

		if (m_has_backward)
		{
			decode_motion_vector(m_motion_bw_h, read_vlc(MOTION), m_backward_r_size, m_backward_f);
			decode_motion_vector(m_motion_bw_v, read_vlc(MOTION), m_backward_r_size, m_backward_f);
		}

		predict_macroblock();
	}

	const int cbp = m_macroblock_intra
		? 0x3f
		: ((m_macroblock_type & MB_PATTERN) ? read_vlc(CODE_BLOCK_PATTERN) : 0);

	for (int block = 0; block < 6; block++)
	{
		if (cbp & (0x20 >> block))
			decode_block(block);
	}

	// A block that could not be read leaves this macroblock part decoded and
	// the bit position wrong, so hand it back for concealment too.
	if (m_slice_error)
		m_macroblock_address--;
}

void mpeg1_video_decoder::decode_motion_vector(int &dst, int motion, int r_size, int f)
{
	int delta;
	if (motion != 0 && f != 1)
	{
		const int r = int(read_bits(r_size));
		delta = ((std::abs(motion) - 1) << r_size) + r + 1;
		if (motion < 0)
			delta = -delta;
	}
	else
	{
		delta = motion;
	}

	dst += delta;
	if (dst > (f << 4) - 1)
		dst -= f << 5;
	else if (dst < ((-f) << 4))
		dst += f << 5;
}

//**************************************************************************
//  Block decoding
//**************************************************************************

void mpeg1_video_decoder::decode_block(int block)
{
	std::memset(m_block_data, 0, sizeof(m_block_data));

	int n = 0;

	if (m_macroblock_intra)
	{
		const int plane = (block > 3) ? (block - 3) : 0;
		int predictor = m_dc_predictor[plane];

		const int dct_size = read_vlc(DCT_SIZE[plane]);
		if (dct_size > 0)
		{
			const int differential = int(read_bits(dct_size));
			if (differential & (1 << (dct_size - 1)))
				predictor += differential;
			else
				predictor += (-1 << dct_size) | (differential + 1);
		}

		m_dc_predictor[plane] = predictor;
		m_block_data[0] = predictor << 3;   // intra DC scaler is 8
		n = 1;
	}

	for (;;)
	{
		const uint16_t coeff = read_vlc_uint(DCT_COEFF);
		if (coeff == 0)
		{
			// No run/level pair packs to zero, so this is a code the table
			// cannot produce and the block is being read at the wrong offset.
			m_slice_error = true;
			break;
		}

		int run, level;

		if (coeff == 0x0001 && n > 0)
		{
			// A leading 1 bit is either end of block (10) or a run of zero
			// with level one (11s).
			if (read_bit() == 0)
				break;
			run = 0;
			level = read_bit() ? -1 : 1;
		}
		else if (coeff == DCT_COEFF_ESCAPE)
		{
			run = int(read_bits(6));
			level = int(read_bits(8));
			if (level == 0)
				level = int(read_bits(8));
			else if (level == 128)
				level = int(read_bits(8)) - 256;
			else if (level > 128)
				level -= 256;
		}
		else
		{
			run = coeff >> 8;
			level = coeff & 0xff;
			if (read_bit())
				level = -level;
		}

		n += run;
		if (n >= 64)
		{
			// A run that carries past the end of the block cannot be coded, so
			// the same applies.
			m_slice_error = true;
			break;
		}

		const int index = ZIG_ZAG[n];
		n++;

		// Dequantise. The result is forced odd, which is what the standard
		// calls for to avoid accumulating a DC bias.
		int value;
		if (m_macroblock_intra)
		{
			value = (2 * level * m_quantizer_scale * m_intra_quant[index]) / 16;
		}
		else
		{
			const int magnitude = std::abs(level);
			value = (((2 * magnitude) + 1) * m_quantizer_scale * m_non_intra_quant[index]) / 16;
			if (level < 0)
				value = -value;
		}

		// The standard forces the magnitude odd, but Sign(0) is zero, so a
		// coefficient that dequantises to zero must be left alone.
		if (value != 0 && (value & 1) == 0)
			value -= (value > 0) ? 1 : -1;

		value = std::max(-2048, std::min(2047, value));

		m_block_data[index] = value;
	}

	// The DC-only shortcut in the row pass needs the DC term left alone, so
	// run the full transform unconditionally.
	idct(m_block_data);

	// Write the result into the current frame, adding to the prediction for
	// non-intra macroblocks.
	uint8_t *dst;
	int stride, x, y;

	if (block < 4)
	{
		dst = m_work->y.data();
		stride = m_luma_width;
		x = m_mb_col * 16 + ((block & 1) * 8);
		y = m_mb_row * 16 + ((block >> 1) * 8);
	}
	else
	{
		dst = (block == 4) ? m_work->cb.data() : m_work->cr.data();
		stride = m_chroma_width;
		x = m_mb_col * 8;
		y = m_mb_row * 8;
	}

	uint8_t *out = dst + size_t(y) * stride + x;

	if (m_macroblock_intra)
	{
		for (int row = 0; row < 8; row++)
			for (int col = 0; col < 8; col++)
				out[size_t(row) * stride + col] = clamp_pixel(m_block_data[row * 8 + col]);
	}
	else
	{
		for (int row = 0; row < 8; row++)
			for (int col = 0; col < 8; col++)
			{
				uint8_t &p = out[size_t(row) * stride + col];
				p = clamp_pixel(int(p) + m_block_data[row * 8 + col]);
			}
	}
}

void mpeg1_video_decoder::idct(int *block)
{
	for (int i = 0; i < 8; i++)
		idct_row(block + i * 8);
	for (int i = 0; i < 8; i++)
		idct_col(block + i);
}

//**************************************************************************
//  Motion compensation
//**************************************************************************

void mpeg1_video_decoder::predict_macroblock()
{
	// A P picture predicts from the most recent anchor, which has not yet been
	// rotated into the past slot, so it is m_future. A B picture sits between
	// the two anchors and uses m_past looking forwards and m_future looking
	// backwards.
	const frame *const forward_ref = (m_picture_type == PICTURE_TYPE_B) ? m_past : m_future;

	// Test the directions actually used by this macroblock. A leading B
	// picture that only predicts backwards can use the new I anchor, while
	// one that also needs the missing past anchor cannot be reconstructed.
	if (((m_has_forward || !m_has_backward) && !forward_ref->valid) ||
		(m_has_backward && !m_future->valid))
	{
		m_work->valid = false;
		return;
	}

	if (m_has_forward && m_has_backward)
		interpolate_macroblock(*forward_ref, m_motion_fw_h, m_motion_fw_v, *m_future, m_motion_bw_h, m_motion_bw_v);
	else if (m_has_backward)
		copy_macroblock(*m_future, m_motion_bw_h, m_motion_bw_v, m_full_pel_backward);
	else
		copy_macroblock(*forward_ref, m_motion_fw_h, m_motion_fw_v, m_full_pel_forward);
}

namespace {

// Copies one 8x8 or 16x16 region with half-pel interpolation.
void copy_plane(uint8_t *dst, const uint8_t *src, int stride, int width, int height,
	int dst_x, int dst_y, int src_x, int src_y, bool half_x, bool half_y)
{
	for (int row = 0; row < height; row++)
	{
		for (int col = 0; col < width; col++)
		{
			const uint8_t *p = src + size_t(src_y + row) * stride + src_x + col;

			int value;
			if (half_x && half_y)
				value = (int(p[0]) + int(p[1]) + int(p[stride]) + int(p[stride + 1]) + 2) / 4;
			else if (half_x)
				value = (int(p[0]) + int(p[1]) + 1) / 2;
			else if (half_y)
				value = (int(p[0]) + int(p[stride]) + 1) / 2;
			else
				value = p[0];

			dst[size_t(dst_y + row) * stride + dst_x + col] = uint8_t(value);
		}
	}
}

// As above, but averages two predictions.
void interpolate_plane(uint8_t *dst, const uint8_t *a, const uint8_t *b, int stride,
	int width, int height, int dst_x, int dst_y,
	int ax, int ay, bool a_half_x, bool a_half_y,
	int bx, int by, bool b_half_x, bool b_half_y)
{
	for (int row = 0; row < height; row++)
	{
		for (int col = 0; col < width; col++)
		{
			const uint8_t *pa = a + size_t(ay + row) * stride + ax + col;
			const uint8_t *pb = b + size_t(by + row) * stride + bx + col;

			int va;
			if (a_half_x && a_half_y)
				va = (int(pa[0]) + int(pa[1]) + int(pa[stride]) + int(pa[stride + 1]) + 2) / 4;
			else if (a_half_x)
				va = (int(pa[0]) + int(pa[1]) + 1) / 2;
			else if (a_half_y)
				va = (int(pa[0]) + int(pa[stride]) + 1) / 2;
			else
				va = pa[0];

			int vb;
			if (b_half_x && b_half_y)
				vb = (int(pb[0]) + int(pb[1]) + int(pb[stride]) + int(pb[stride + 1]) + 2) / 4;
			else if (b_half_x)
				vb = (int(pb[0]) + int(pb[1]) + 1) / 2;
			else if (b_half_y)
				vb = (int(pb[0]) + int(pb[stride]) + 1) / 2;
			else
				vb = pb[0];

			dst[size_t(dst_y + row) * stride + dst_x + col] = uint8_t((va + vb + 1) / 2);
		}
	}
}

// Clamps a source position so that interpolation never reads outside the plane.
inline void clamp_source(int &pos, bool &half, int limit, int size)
{
	if (pos < 0)
	{
		pos = 0;
		half = false;
	}
	else if (pos + size + (half ? 1 : 0) > limit)
	{
		pos = std::max(0, limit - size - (half ? 1 : 0));
		if (pos + size + (half ? 1 : 0) > limit)
			half = false;
	}
}

} // anonymous namespace

void mpeg1_video_decoder::copy_macroblock(const frame &src, int motion_h, int motion_v, bool full_pel)
{
	if (src.y.empty())
		return;

	// A full-pel vector counts whole samples, so scale it into the half-pel
	// units the interpolation below works in.
	const int mh = full_pel ? motion_h * 2 : motion_h;
	const int mv = full_pel ? motion_v * 2 : motion_v;

	// Luma
	{
		int sx = m_mb_col * 16 + (mh >> 1);
		int sy = m_mb_row * 16 + (mv >> 1);
		bool hx = (mh & 1) != 0;
		bool hy = (mv & 1) != 0;

		clamp_source(sx, hx, m_luma_width, 16);
		clamp_source(sy, hy, m_luma_height, 16);

		copy_plane(m_work->y.data(), src.y.data(), m_luma_width, 16, 16,
			m_mb_col * 16, m_mb_row * 16, sx, sy, hx, hy);
	}

	// Chroma: the vectors are halved, truncating toward zero.
	{
		const int ch = mh / 2;
		const int cv = mv / 2;

		int sx = m_mb_col * 8 + (ch >> 1);
		int sy = m_mb_row * 8 + (cv >> 1);
		bool hx = (ch & 1) != 0;
		bool hy = (cv & 1) != 0;

		clamp_source(sx, hx, m_chroma_width, 8);
		clamp_source(sy, hy, m_chroma_height, 8);

		copy_plane(m_work->cb.data(), src.cb.data(), m_chroma_width, 8, 8,
			m_mb_col * 8, m_mb_row * 8, sx, sy, hx, hy);
		copy_plane(m_work->cr.data(), src.cr.data(), m_chroma_width, 8, 8,
			m_mb_col * 8, m_mb_row * 8, sx, sy, hx, hy);
	}
}

void mpeg1_video_decoder::interpolate_macroblock(const frame &fwd, int fwd_h, int fwd_v,
	const frame &bwd, int bwd_h, int bwd_v)
{
	if (fwd.y.empty() || bwd.y.empty())
		return;

	const int fh = m_full_pel_forward ? fwd_h * 2 : fwd_h;
	const int fv = m_full_pel_forward ? fwd_v * 2 : fwd_v;
	const int bh = m_full_pel_backward ? bwd_h * 2 : bwd_h;
	const int bv = m_full_pel_backward ? bwd_v * 2 : bwd_v;

	// Luma
	{
		int fx = m_mb_col * 16 + (fh >> 1), fy = m_mb_row * 16 + (fv >> 1);
		int bx = m_mb_col * 16 + (bh >> 1), by = m_mb_row * 16 + (bv >> 1);
		bool fhx = (fh & 1) != 0, fhy = (fv & 1) != 0;
		bool bhx = (bh & 1) != 0, bhy = (bv & 1) != 0;

		clamp_source(fx, fhx, m_luma_width, 16);
		clamp_source(fy, fhy, m_luma_height, 16);
		clamp_source(bx, bhx, m_luma_width, 16);
		clamp_source(by, bhy, m_luma_height, 16);

		interpolate_plane(m_work->y.data(), fwd.y.data(), bwd.y.data(), m_luma_width,
			16, 16, m_mb_col * 16, m_mb_row * 16, fx, fy, fhx, fhy, bx, by, bhx, bhy);
	}

	// Chroma
	{
		const int fch = fh / 2, fcv = fv / 2;
		const int bch = bh / 2, bcv = bv / 2;

		int fx = m_mb_col * 8 + (fch >> 1), fy = m_mb_row * 8 + (fcv >> 1);
		int bx = m_mb_col * 8 + (bch >> 1), by = m_mb_row * 8 + (bcv >> 1);
		bool fhx = (fch & 1) != 0, fhy = (fcv & 1) != 0;
		bool bhx = (bch & 1) != 0, bhy = (bcv & 1) != 0;

		clamp_source(fx, fhx, m_chroma_width, 8);
		clamp_source(fy, fhy, m_chroma_height, 8);
		clamp_source(bx, bhx, m_chroma_width, 8);
		clamp_source(by, bhy, m_chroma_height, 8);

		interpolate_plane(m_work->cb.data(), fwd.cb.data(), bwd.cb.data(), m_chroma_width,
			8, 8, m_mb_col * 8, m_mb_row * 8, fx, fy, fhx, fhy, bx, by, bhx, bhy);
		interpolate_plane(m_work->cr.data(), fwd.cr.data(), bwd.cr.data(), m_chroma_width,
			8, 8, m_mb_col * 8, m_mb_row * 8, fx, fy, fhx, fhy, bx, by, bhx, bhy);
	}
}

//**************************************************************************
//  Output
//**************************************************************************

const uint8_t *mpeg1_video_decoder::display_y() const
{
	return m_display_frame ? m_display_frame->y.data() : nullptr;
}

const uint8_t *mpeg1_video_decoder::display_cb() const
{
	return m_display_frame ? m_display_frame->cb.data() : nullptr;
}

const uint8_t *mpeg1_video_decoder::display_cr() const
{
	return m_display_frame ? m_display_frame->cr.data() : nullptr;
}

void mpeg1_video_decoder::display_frame_rgb(uint32_t *dst, int stride) const
{
	if (!m_display_frame || m_display_frame->y.empty())
		return;

	const uint8_t *y = m_display_frame->y.data();
	const uint8_t *cb = m_display_frame->cb.data();
	const uint8_t *cr = m_display_frame->cr.data();

	for (int row = 0; row < m_height; row++)
	{
		const uint8_t *yr = y + size_t(row) * m_luma_width;
		const uint8_t *cbr = cb + size_t(row / 2) * m_chroma_width;
		const uint8_t *crr = cr + size_t(row / 2) * m_chroma_width;
		uint32_t *out = dst + size_t(row) * stride;

		for (int col = 0; col < m_width; col++)
		{
			// BT.601, but deliberately *not* expanded to full range: the
			// picture is handed to the MCD212 as External Video, and every
			// other pixel that chip emits carries the CD-i's 16-235 studio
			// swing (see s_4bpp_color[] and the -16/+16 framing in
			// mix_lines()).  Expanding here would put the cartridge's black at
			// 0 while the planes around it sit at 16, and the display window
			// would then show up as a darker rectangle against the black the
			// title draws around it - most visibly for the couple of fields at
			// the start of a clip where the window is open but no picture has
			// been shown yet.  On hardware the cartridge feeds the same mixer
			// and the same DAC as the planes, so its black *is* the CD-i's
			// black.
			//
			// So the luma passes through with its pedestal intact and only the
			// chroma coefficients are scaled by 219/255 to match.  Y=16 gives
			// 16 and Y=235 gives 235; coded values outside the nominal range
			// still land outside it, the same way the DYUV decoder's limit
			// table lets them.
			const int base = (int(yr[col]) << 16) + 32768;
			const int u = int(cbr[col / 2]) - 128;
			const int v = int(crr[col / 2]) - 128;

			const int r = (base + 89827 * v) >> 16;
			const int g = (base - 22049 * u - 45758 * v) >> 16;
			const int b = (base + 113538 * u) >> 16;

			out[col] = (uint32_t(clamp_pixel(r)) << 16)
					 | (uint32_t(clamp_pixel(g)) << 8)
					 |  uint32_t(clamp_pixel(b));
		}
	}
}
