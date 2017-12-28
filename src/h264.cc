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

#include "h264.hh"

using std::shared_ptr;
using std::runtime_error;

h264::h264(MP4File &mp4) {
    auto tracks = mp4.find_all("trak");
    std::shared_ptr<Box> box = nullptr;
    for (const auto & track : tracks) {
        auto b = track->find_first("avc1");
        if (b) {
            box = b;
            break;
        }
    }
    shared_ptr<PPS_NALUnit> pps;
    shared_ptr<SPS_NALUnit> sps;
    if (box) {
        Avc1 avc1 = Avc1(*box.get());
        auto pps_list = avc1.avcC().pps_units();
        auto sps_list = avc1.avcC().sps_units();
        pps = pps_list[0];
        sps = sps_list[0]; /* use the first one */
    }
    if (!pps || !sps)
        throw std::runtime_error("sps or pps not found in mp4 file");
    for (const auto & b : mp4.find_all("mdat")) {
        MdatBox mdat(b);
        for (auto & nal : mdat.nal_units()) {
            if (nal->nal_unit_type() <= 5 and nal->nal_unit_type() >= 1) {
                Slice_NALUnit slice(nal);
                slice.parse(sps, pps);
                std::cout << slice.header().slice_qp_delta << std::endl;
                //break;
            }
        }
    }
}
