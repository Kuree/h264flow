//
// Created by keyi on 12/24/17.
//

#include "h264.hh"

using std::shared_ptr;
using std::runtime_error;

h264::h264(MP4File &mp4) {
    auto box = mp4.find_first("avc1");
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
