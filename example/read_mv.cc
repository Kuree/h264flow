//
// Created by keyi on 12/21/17.
//

#include "../src/h264.hh"

using namespace std;

int main(int argc, char * argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <file_name> <frame_number>" << endl;
        return EXIT_FAILURE;
    }
    std::string filename = argv[1];
    uint32_t frame_num = (uint32_t)stoi(argv[2]);
    cerr << "WARN: MPEG-4/AVC is untested" << endl;
    auto bs = make_shared<BitStream>(filename);
    h264 decoder(bs);
    std::shared_ptr<MvFrame> frame = decoder.load_frame(frame_num);
    auto mv = frame->get_mv(6);
    cout << mv.mvL0[0] << " " << mv.mvL0[1] << endl;
}
