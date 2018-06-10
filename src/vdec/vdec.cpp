extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}
#include "vdec.h"

using namespace std;

const char * VDEC::test() {
    return avcodec_configuration();
}
