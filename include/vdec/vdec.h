#pragma once

#ifdef VDEC_EXPORTS
#define VDEC_API __declspec(dllexport)
#else
#define VDEC_API __declspec(dllimport)
#endif

class VDEC_API VDEC {
public:
    const char *test();
};
