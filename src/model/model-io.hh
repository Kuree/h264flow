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

#ifndef H264FLOW_MODEL_IO_HH
#define H264FLOW_MODEL_IO_HH

#include "../decoder/h264.hh"

void dump_mv(const MvFrame &frame, uint32_t label, std::string filename);

std::tuple<MvFrame, uint32_t> load_mv(std::string filename);

uint32_t create_label(bool left, bool right, bool up, bool down,
                      bool zoom_in, bool zoom_out);

void load_label(uint32_t label, bool &left, bool &right,
                bool &up, bool &down, bool &zoom_in, bool &zoom_out);

void dump_processed_mv(const MvFrame &frame, uint32_t label,
                       std::string filename);

std::vector<double> load_processed_mv(const std::string & filename,
                                      uint32_t &label);

void dump_av(const std::vector<std::vector<std::pair<int, int>>> & mvs,
             const std::vector<uint8_t> &luma, std::string filename);

std::pair<std::vector<std::vector<std::pair<int, int>>>, std::vector<uint8_t>>
load_av(const std::string &filename);


/* this is for Sintel MPI */
std::vector<std::vector<std::pair<float, float>>>
load_sintel_flo(const std::string &filename);

#endif //H264FLOW_MODEL_IO_HH
