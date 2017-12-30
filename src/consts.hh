/*  This file is part of h264flow.

h264flow is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

h264flow is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with h264flow.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef H264FLOW_CONSTS_HH
#define H264FLOW_CONSTS_HH

/* taken from https://github.com/zoltanmaric/h264-fer */
#define P_L0_16x16      0
#define P_L0_L0_16x8    1
#define P_L0_L0_8x16    2
#define P_8x8           3
#define P_8x8ref0       4
#define I_4x4           5
#define I_16x16_0_0_0   6
#define I_16x16_1_0_0   7
#define I_16x16_2_0_0   8
#define I_16x16_3_0_0   9
#define I_16x16_0_1_0  10
#define I_16x16_1_1_0  11
#define I_16x16_2_1_0  12
#define I_16x16_3_1_0  13
#define I_16x16_0_2_0  14
#define I_16x16_1_2_0  15
#define I_16x16_2_2_0  16
#define I_16x16_3_2_0  17
#define I_16x16_0_0_1  18
#define I_16x16_1_0_1  19
#define I_16x16_2_0_1  20
#define I_16x16_3_0_1  21
#define I_16x16_0_1_1  22
#define I_16x16_1_1_1  23
#define I_16x16_2_1_1  24
#define I_16x16_3_1_1  25
#define I_16x16_0_2_1  26
#define I_16x16_1_2_1  27
#define I_16x16_2_2_1  28
#define I_16x16_3_2_1  29
#define I_PCM          30
#define NA              0
#define P_Skip         31
#define Intra_4x4       0
#define Intra_16x16     1
#define Pred_L0         2
#define Pred_L1         3
#define BiPred          4
#define Direct          5

#define P_L0_8x8       0
#define P_L0_8x4       1
#define P_L0_4x8       2
#define P_L0_4x4       3

#define B_Direct_8x8    60 // 0
#define B_L0_8x8        61
#define B_L1_8x8        62
#define B_Bi_8x8        63
#define B_L0_8x4        64
#define B_L0_4x8        65
#define B_L1_8x4        66
#define B_L1_4x8        67
#define B_Bi_8x4        68
#define B_Bi_4x8        69
#define B_L0_4x4        70
#define B_L1_4x4        71
#define B_Bi_4x4        72  // 12

#define I_NxN 0

//Inter prediction slices - Macroblock types
//Defined strictly by norm, page 121.
//(Table 7-13 – Macroblock type values 0 to 4 for P and SP slices)
/*
First column:	mb_type
Second column:	Name of mb_type
Third column:	NumMbPart( mb_type )
Fourth column:	MbPartPredMode( mb_type, 0 )
Fifth column:	MbPartPredMode( mb_type, 1 )
Sixth column:	MbPartWidth( mb_type )
Seventh column:	MbPartHeight( mb_type )
*/
uint32_t P_and_SP_macroblock_modes[32][7]= {
        {0,  P_L0_16x16,    1,      Pred_L0,      NA,    16,  16},
        {1,  P_L0_L0_16x8,  2,      Pred_L0, Pred_L0,    16,  8},
        {2,  P_L0_L0_8x16,  2,      Pred_L0, Pred_L0,     8,  16},
        {3,  P_8x8,         4,           NA,      NA,     8,  8},
        {4,  P_8x8ref0,     4,           NA,      NA,     8,  8},
        {0,  I_4x4,         0,    Intra_4x4,      NA,    NA, NA},
        {1,  I_16x16_0_0_0, NA, Intra_16x16,       0,     0,  0},
        {2,  I_16x16_1_0_0, NA, Intra_16x16,       1,     0,  0},
        {3,  I_16x16_2_0_0, NA, Intra_16x16,       2,     0,  0},
        {4,  I_16x16_3_0_0, NA, Intra_16x16,       3,     0,  0},
        {5,  I_16x16_0_1_0, NA, Intra_16x16,       0,     1,  0},
        {6,  I_16x16_1_1_0, NA, Intra_16x16,       1,     1,  0},
        {7,  I_16x16_2_1_0, NA, Intra_16x16,       2,     1,  0},
        {8,  I_16x16_3_1_0, NA, Intra_16x16,       3,     1,  0},
        {9,  I_16x16_0_2_0, NA, Intra_16x16,       0,     2,  0},
        {10, I_16x16_1_2_0, NA, Intra_16x16,       1,     2,  0},
        {11, I_16x16_2_2_0, NA, Intra_16x16,       2,     2,  0},
        {12, I_16x16_3_2_0, NA, Intra_16x16,       3,     2,  0},
        {13, I_16x16_0_0_1, NA, Intra_16x16,       0,     0,  15},
        {14, I_16x16_1_0_1, NA, Intra_16x16,       1,     0,  15},
        {15, I_16x16_2_0_1, NA, Intra_16x16,       2,     0,  15},
        {16, I_16x16_3_0_1, NA, Intra_16x16,       3,     0,  15},
        {17, I_16x16_0_1_1, NA, Intra_16x16,       0,     1,  15},
        {18, I_16x16_1_1_1, NA, Intra_16x16,       1,     1,  15},
        {19, I_16x16_2_1_1, NA, Intra_16x16,       2,     1,  15},
        {20, I_16x16_3_1_1, NA, Intra_16x16,       3,     1,  15},
        {21, I_16x16_0_2_1, NA, Intra_16x16,       0,     2,  15},
        {22, I_16x16_1_2_1, NA, Intra_16x16,       1,     2,  15},
        {23, I_16x16_2_2_1, NA, Intra_16x16,       2,     2,  15},
        {24, I_16x16_3_2_1, NA, Intra_16x16,       3,     2,  15},
        {25, I_PCM,         NA,          NA,      NA,    NA,  NA},
        {NA, P_Skip,         1,     Pred_L0,      NA,    16,  16}
};

//Intra prediction slices - Macroblock types
//Defined strictly by norm, page 119.
//(Table 7-11 – Macroblock types for I slices)
/*
First column:	mb_type
Second column:	Name of mb_type
Third column:	tranform_size_8x8_flag
Fourth column:	MbPartPredMode( mb_type, 0 )
Fifth column:	Intra16x16PredMode
Sixth column:	CodedBlockPatternChroma
Seventh column:	CodedBlockPatternLuma
*/

//Intra prediction slices - Macroblock types
//Defined strictly by norm, page 119.
//(Table 7-11 – Macroblock types for I slices)
/*
First column:	mb_type
Second column:	Name of mb_type
Third column:	tranform_size_8x8_flag
Fourth column:	MbPartPredMode( mb_type, 0 )
Fifth column:	Intra16x16PredMode
Sixth column:	CodedBlockPatternChroma
Seventh column:	CodedBlockPatternLuma
*/

uint32_t I_Macroblock_Modes[27][7]=
{
  {0,	I_4x4,			0,	Intra_4x4,   NA, NA, NA},
  //If this line was to be commented out, the MbPartPredMode macro would have to be changed
  //since it relies on the linear rise of the value in the first column.
  //{0,	I_NxN,			1,	Intra_8x8,	 NA, NA, NA},
  {1,	I_16x16_0_0_0,	NA,	Intra_16x16,  0,  0,  0},
  {2,	I_16x16_1_0_0,	NA,	Intra_16x16,  1,  0,  0},
  {3,	I_16x16_2_0_0,	NA,	Intra_16x16,  2,  0,  0},
  {4,	I_16x16_3_0_0,	NA,	Intra_16x16,  3,  0,  0},
  {5,	I_16x16_0_1_0,	NA,	Intra_16x16,  0,  1,  0},
  {6,	I_16x16_1_1_0,	NA,	Intra_16x16,  1,  1,  0},
  {7,	I_16x16_2_1_0,	NA,	Intra_16x16,  2,  1,  0},
  {8,	I_16x16_3_1_0,	NA,	Intra_16x16,  3,  1,  0},
  {9,	I_16x16_0_2_0,	NA,	Intra_16x16,  0,  2,  0},
  {10,	I_16x16_1_2_0,	NA,	Intra_16x16,  1,  2,  0},
  {11,	I_16x16_2_2_0,	NA,	Intra_16x16,  2,  2,  0},
  {12,	I_16x16_3_2_0,	NA,	Intra_16x16,  3,  2,  0},
  {13,	I_16x16_0_0_1,	NA,	Intra_16x16,  0,  0, 15},
  {14,	I_16x16_1_0_1,	NA,	Intra_16x16,  1,  0, 15},
  {15,	I_16x16_2_0_1,	NA,	Intra_16x16,  2,  0, 15},
  {16,	I_16x16_3_0_1,	NA,	Intra_16x16,  3,  0, 15},
  {17,	I_16x16_0_1_1,	NA,	Intra_16x16,  0,  1, 15},
  {18,	I_16x16_1_1_1,	NA,	Intra_16x16,  1,  1, 15},
  {19,	I_16x16_2_1_1,	NA,	Intra_16x16,  2,  1, 15},
  {20,	I_16x16_3_1_1,	NA,	Intra_16x16,  3,  1, 15},
  {21,	I_16x16_0_2_1,	NA,	Intra_16x16,  0,  2, 15},
  {22,	I_16x16_1_2_1,	NA,	Intra_16x16,  1,  2, 15},
  {23,	I_16x16_2_2_1,	NA,	Intra_16x16,  2,  2, 15},
  {24,	I_16x16_3_2_1,	NA,	Intra_16x16,  3,  2, 15},
  {25,	I_PCM,			NA,	NA,			  NA, NA, NA}
};

//Inter prediction data for submacroblocks
//Defined strictly by the norm
//(Table 7-17 – Sub-macroblock types in P macroblocks)
/*
First column:	sub_mb_type[ mbPartIdx ]
Second coumn:	Name of sub_mb_type[ mbPartIdx ]
Third column:	NumSubMbPart( sub_mb_type[ mbPartIdx ] )
Fourth column:	SubMbPredMode( sub_mb_type[ mbPartIdx ] )
Fifth column:	SubMbPartWidth( sub_mb_type[ mbPartIdx ] )
Sixth column:	SubMbPartHeight( sub_mb_type[ mbPartIdx ] )
*/
uint32_t P_sub_macroblock_modes[4][6] = {
	{0,		P_L0_8x8,			1,	Pred_L0, 8,		8},
	{1,		P_L0_8x4,			2,	Pred_L0, 8,		4},
	{2,		P_L0_4x8,			2,	Pred_L0, 4,		8},
	{3,		P_L0_4x4,			4,	Pred_L0, 4,		4}
};

uint32_t B_sub_macroblock_modes[13][6] = {
		{0, B_Direct_8x8, NA,   Direct, 4,  4},
		{1,     B_L0_8x8,  1,  Pred_L0, 8,  8},
		{2,     B_L1_8x8,  1,  Pred_L1, 8,  8},
		{3,     B_Bi_8x8,  1,   BiPred, 8,  8},
		{4,     B_L0_8x4,  2,  Pred_L0, 8,  4},
		{5,     B_L0_4x8,  2,  Pred_L0, 4,  8},
		{6,     B_L1_8x4,  2,  Pred_L1, 8,  4},
		{7,     B_L1_4x8,  2,  Pred_L1, 4,  8},
		{8,     B_Bi_8x4,  2,   BiPred, 8,  4},
		{9,     B_Bi_4x8,  2,   BiPred, 4,  8},
		{10,    B_L0_4x4,  4,  Pred_L0, 4,  4},
		{11,    B_L1_4x4,  4,  Pred_L1, 4,  4},
		{12,    B_Bi_4x4,  4,   BiPred, 4,  4},
};



int MbPartPredMode(uint64_t mb_type, uint64_t x, uint64_t slice_type) {

    return (slice_type % 5 == 0) ?
           P_and_SP_macroblock_modes[mb_type][3 + (x % 2)] :
           I_Macroblock_Modes[mb_type][3];
}

uint64_t NumMbPart(uint64_t mb_type) {
    return P_and_SP_macroblock_modes[mb_type][2];
}

int codeNum_to_coded_block_pattern_intra[48]= {
        47, 31, 15, 0, 23, 27, 29, 30, 7, 11, 13, 14, 39, 43, 45, 46,
        16, 3, 5, 10, 12, 19, 21, 26, 28, 35, 37, 42, 44, 1, 2, 4,
        8, 17, 18, 20, 24, 6, 9, 22, 25, 32, 33, 34, 36, 40, 38, 41
};

uint64_t NumSubMbPart(uint64_t sub_mb_type, uint64_t slice_type) {
    /* TODO: might not be true */
    if (slice_type == SliceType::TYPE_P) {
        return P_sub_macroblock_modes[sub_mb_type][2];
    } else if (slice_type == SliceType::TYPE_B) {
        return B_sub_macroblock_modes[sub_mb_type][2];
    } else {
        throw std::runtime_error("unsupported slice_type for sub_mb_type");
    }
}

uint64_t SubMbPredMode(uint64_t sub_mb_type, uint64_t slice_type) {
    if (slice_type == SliceType::TYPE_P) {
        return P_sub_macroblock_modes[sub_mb_type][3];
    } else if (slice_type == SliceType::TYPE_B) {
        return B_sub_macroblock_modes[sub_mb_type][3];
    } else {
        throw std::runtime_error("unsupported slice_type for sub_mb_type");
    }
}

#endif //H264FLOW_CONSTS_HH
