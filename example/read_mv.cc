//
// Created by keyi on 12/21/17.
//

#include "../src/h264.hh"
#include "../src/util.hh"

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
    try {
        std::shared_ptr<MvFrame> frame = decoder.load_frame(frame_num);
        cout << "Frame size: " << frame->width() << "x" << frame->height()
             << endl;
        uint32_t counter = 0;
        for (uint32_t y = 0; y < frame->mb_height(); y++) {
            for (uint32_t x = 0; x < frame->mb_width(); x++) {
                auto mv = frame->get_mv(x, y);
                cout << "x: " << x << " y: " << y << " mvL0: ("
                     <<  mv.mvL0[0] << ", " << mv.mvL0[1] << ")";
                if (counter++ % 3 == 2)
                    cout << endl;
                else
                    cout << "    ";
            }
        }
    } catch (const NotImplemented & ex) {
        cerr << "ERROR " << ex.what()
             << endl << "Please make sure it's a P-slice" << endl;
    }
}
