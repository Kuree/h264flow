//
// Created by keyi on 12/21/17.
//

#include "../src/h264.hh"

using namespace std;

int main(int , char * []) {
    auto bs = make_shared<BitStream>("news.264");
    h264 decoder(bs);
    std::shared_ptr<MvFrame> frame = decoder.load_frame(4);
    auto mv = frame->get_mv(6);
    cout << mv.mvL0[0] << " " << mv.mvL0[1] << endl;
}
