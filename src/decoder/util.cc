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

#include "util.hh"
#include "consts.hh"

uint64_t intlog2(uint64_t x)
{
    uint64_t log = 0;
    while ((x >> log) > 0)
    {
        log++;
    }
    if (x == 1u<<(log-1)) { log--; }
    return log;
}

uint8_t read_coeff_token(int nC, BitReader & br) {
    /* adapted from https://goo.gl/pWSEqT */
    if (nC >= 8) {
        auto code = static_cast<uint32_t>(br.read_bits(6));
        int TotalCoeff   = (code >> 2);
        int TrailingOnes = (code & 3);
        if (TotalCoeff == 0 && TrailingOnes == 3)
            TrailingOnes = 0;
        else
            TotalCoeff++;
        return (uint8_t)((TotalCoeff << 2) | (TrailingOnes));
    }

    int tab = (nC == -2) ? 4 : (nC == -1) ? 3 : (nC < 2) ? 0 : (nC < 4) ? 1 : (nC < 8) ? 2 : 5;

    /* optimize next bits to save disk seek */
    //uint64_t next_bytes = br.next_bits(16);
    int64_t bits_left = br.bits_left();
    for (int TrailingOnes = 0; TrailingOnes < 4; TrailingOnes++) {
        for (int TotalCoeff = 0; TotalCoeff < 17; TotalCoeff++) {
            uint32_t length = coeff_token_length[tab][TrailingOnes][TotalCoeff];
            uint32_t code   = coeff_token_code[tab][TrailingOnes][TotalCoeff];
            //uint64_t next_bits = next_bytes >> (16 - length);
            if ((int)length > (bits_left - 1)) continue; /* 1 bit for stopping bit */
            uint64_t next_bits = br.next_bits(length);
            if (length > 0 && next_bits == code) {
                br.read_bits(length);
                return (uint8_t)((TotalCoeff << 2) | (TrailingOnes));
            }
        }
    }
    throw std::runtime_error("coeff_token not found");
}

uint8_t read_ce_levelprefix(BitReader &br) {
    int leadingZeroBits = -1;
    for (int b = 0; !b; leadingZeroBits++)
        b = br.read_bit();
    return static_cast<uint8_t>(leadingZeroBits);
}

int code_from_bitstream_2d(BitReader &br,
                           const uint8_t *lentab, const uint8_t *codtab,
                           const int tabwidth, const int tabheight) {
    const uint8_t *len = &lentab[0];
    const uint8_t *cod = &codtab[0];
    int64_t bits_left = br.bits_left() - 1; /* 1 for stopping bit */
    for (int j = 0; j < tabheight; j++) {
        for (int i = 0; i < tabwidth; i++) {
            if (*len > bits_left) continue;
            uint64_t next_bits = br.next_bits(*len);
            if ((*len == 0) || (next_bits != *cod)) {
                ++len;
                ++cod;
            } else {
                // Move bitstream pointer
                br.read_bits(*len);

                // The syntax element value
                return i;
            }
        }
    }

    throw std::runtime_error("unable to decode bit stream 2d");
}

int read_ce_totalzeros(BitReader &br, const int vlcnum,
                       const int chromadc) {
    if (!chromadc)
        return code_from_bitstream_2d(br, &totalzeros_lentab[vlcnum][0],
                                      &totalzeros_codtab[vlcnum][0], 16, 1);
    else
        return  code_from_bitstream_2d(br,
                                       &totalzeros_chromadc_lentab[0][vlcnum][0],
                                       &totalzeros_chromadc_codtab[0][vlcnum][0],
                                       4, 1);
}

int read_ce_runbefore(BitReader &br, const int vlcnum) {
    return code_from_bitstream_2d(br, &runbefore_lentab[vlcnum][0],
                                  &runbefore_codtab[vlcnum][0], 16, 1);
}

void read_rbsp_trailing_bits(BinaryReader &br) {
    uint8_t stop_bit =  br.read_bit();
    if (!stop_bit)
        throw std::runtime_error("stop bit is not 1");
    br.reset_bit(true);
}

bool is_mb_intra(uint64_t mb_type, uint64_t slice_type) {
    int type = MbPartPredMode(mb_type, 0, slice_type);
    return (type == Intra_4x4 || type == Intra_16x16);
}

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

int MbPredLuma(uint64_t mb_type) {
    return I_Macroblock_Modes[mb_type][6];
}

int MbPredChroma(uint64_t mb_type) {
    return I_Macroblock_Modes[mb_type][5];
}

int MbPartPredMode(uint64_t mb_type, uint64_t x, uint64_t slice_type) {
    return (slice_type == SliceType::TYPE_P) ?
           P_and_SP_macroblock_modes[mb_type][3 + (x % 2)] :
           I_Macroblock_Modes[mb_type][3];
}

uint64_t NumMbPart(uint64_t mb_type) {
    return P_and_SP_macroblock_modes[mb_type][2];
}

uint64_t MbPartWidth(uint64_t mb_type) {
    return P_and_SP_macroblock_modes[mb_type][5];
}

uint64_t MbPartHeight(uint64_t mb_type) {
    return P_and_SP_macroblock_modes[mb_type][6];
}

bool is_p_slice(uint8_t first_byte, uint8_t second_byte) {
    if (first_byte & 0x80)
        throw std::runtime_error("forbidden 0 bit is set");
    int nal_unit_type = first_byte & 0x1F;
    bool p_slice = (second_byte & 0xFC) == 0x98;
    return nal_unit_type >= 1 && nal_unit_type <= 4 && p_slice;
}