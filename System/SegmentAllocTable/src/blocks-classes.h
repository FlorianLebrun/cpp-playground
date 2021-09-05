#pragma once
#include "./memory-context.h"

namespace sat {

    struct SegmentPaging;
    struct BlockClass;
    struct PageClass;
 
    static const int cSegmentPagingsCount = 48;
    extern const SegmentPaging cSegmentPagings[48];

    static const int cBlockBinCount = 48;
    extern BlockClass* cBlockBinTable[48];
    static const int cBlockClassCount = 73;
    extern BlockClass* cBlockClassTable[73];

    static const int cPageBinCount = 22;
    extern PageClass* cPageBinTable[22];
    static const int cPageClassCount = 25;
    extern PageClass* cPageClassTable[25];

    BlockClass* getBlockClass(size_target_t target);
}
