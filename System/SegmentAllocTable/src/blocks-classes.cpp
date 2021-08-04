
#include "./blocks.h"
#include "./blocks-classes.h"

using namespace sat;

const sat::SegmentPaging sat::cSegmentPagings[48] = {
    { 0, 0 },
    { 0, 4194305 },
    { 0, 2097153 },
    { 0, 1398102 },
    { 0, 1048577 },
    { 0, 838861 },
    { 0, 699051 },
    { 0, 599187 },
    { 0, 524289 },
    { 0, 419431 },
    { 0, 349526 },
    { 0, 299594 },
    { 0, 262145 },
    { 0, 209716 },
    { 0, 174763 },
    { 0, 149797 },
    { 0, 131073 },
    { 0, 104858 },
    { 0, 87382 },
    { 0, 74899 },
    { 0, 65537 },
    { 0, 52429 },
    { 65536, 52429 },
    { 131072, 52429 },
    { 196608, 52429 },
    { 262144, 52429 },
    { 0, 43691 },
    { 65536, 43691 },
    { 131072, 43691 },
    { 0, 37450 },
    { 65536, 37450 },
    { 131072, 37450 },
    { 196608, 37450 },
    { 262144, 37450 },
    { 327680, 37450 },
    { 393216, 37450 },
    { 0, 13108 },
    { 65536, 13108 },
    { 131072, 13108 },
    { 196608, 13108 },
    { 262144, 13108 },
    { 0, 9363 },
    { 65536, 9363 },
    { 131072, 9363 },
    { 196608, 9363 },
    { 262144, 9363 },
    { 327680, 9363 },
    { 393216, 9363 }
};

static sat::PagePnS1Class page_0(0, 0, 1, 1, 10);
static sat::PagePnS1Class page_1(1, 1, 2, 1, 11);
static sat::PagePnS1Class page_2(2, 2, 3, 3, 10);
static sat::PagePnS1Class page_3(3, 3, 4, 1, 12);
static sat::PagePnS1Class page_4(4, 4, 5, 5, 10);
static sat::PagePnS1Class page_5(5, 5, 6, 3, 11);
static sat::PagePnS1Class page_6(6, 6, 7, 7, 10);
static sat::PagePnS1Class page_7(7, 7, 8, 1, 13);
static sat::PagePnS1Class page_8(8, 8, 9, 5, 11);
static sat::PagePnS1Class page_9(9, 9, 10, 3, 12);
static sat::PagePnS1Class page_10(10, 10, 11, 7, 11);
static sat::PagePnS1Class page_11(11, 11, 12, 1, 14);
static sat::PagePnS1Class page_12(12, 12, 13, 5, 12);
static sat::PagePnS1Class page_13(13, 13, 14, 3, 13);
static sat::PagePnS1Class page_14(14, 14, 15, 7, 12);
static sat::PagePnS1Class page_15(15, 15, 16, 1, 15);
static sat::PagePnS1Class page_16(16, 16, 17, 5, 13);
static sat::PagePnS1Class page_17(17, 17, 18, 3, 14);
static sat::PagePnS1Class page_18(18, 18, 19, 7, 13);
static sat::PageP1SnClass page_19(19, 1, 16, {20});
static sat::PagePnSnClass page_20(20, 19, 5, 14, {21,22,23,24,25});
static sat::PagePnSnClass page_21(21, 20, 3, 15, {26,27,28});
static sat::PagePnSnClass page_22(22, 21, 7, 14, {29,30,31,32,33,34,35});
static sat::PageP1SnClass page_23(23, 5, 16, {36,37,38,39,40});
static sat::PageP1SnClass page_24(24, 7, 16, {41,42,43,44,45,46,47});

sat::PageClass* sat::cPageClassTable[25] = {
   &page_0, &page_1, &page_2, &page_3, &page_4, &page_5, &page_6, &page_7,
   &page_8, &page_9, &page_10, &page_11, &page_12, &page_13, &page_14, &page_15,
   &page_16, &page_17, &page_18, &page_19, &page_20, &page_21, &page_22, &page_23,
   &page_24, 
};

static sat::BlockPnS1Class block_0(0, 0, 1, 4, &page_0);
static sat::BlockPnS1Class block_1(1, 1, 1, 5, &page_1);
static sat::BlockPnS1Class block_2(2, 2, 3, 4, &page_2);
static sat::BlockPnS1Class block_3(3, 3, 1, 6, &page_3);
static sat::BlockPnS1Class block_4(4, 4, 5, 4, &page_4);
static sat::BlockPnS1Class block_5(5, 5, 3, 5, &page_5);
static sat::BlockPnS1Class block_6(6, 6, 7, 4, &page_6);
static sat::BlockPnS1Class block_7(7, 7, 1, 7, &page_7);
static sat::BlockPnS1Class block_8(8, 8, 5, 5, &page_8);
static sat::BlockPnS1Class block_9(9, 9, 3, 6, &page_9);
static sat::BlockPnS1Class block_10(10, 10, 7, 5, &page_10);
static sat::BlockPnS1Class block_11(11, 11, 1, 8, &page_11);
static sat::BlockPnS1Class block_12(12, 12, 5, 6, &page_12);
static sat::BlockPnS1Class block_13(13, 13, 3, 7, &page_13);
static sat::BlockPnS1Class block_14(14, 14, 7, 6, &page_14);
static sat::BlockPnS1Class block_15(15, 15, 1, 9, &page_15);
static sat::BlockPnS1Class block_16(16, 16, 5, 7, &page_16);
static sat::BlockPnS1Class block_17(17, 17, 3, 8, &page_17);
static sat::BlockPnS1Class block_18(18, 18, 7, 7, &page_18);
static sat::BlockP1SnClass block_19(19, 19, 1, 10, 6, &page_19);
static sat::BlockPnSnClass block_20(20, 20, 5, 8, 6, &page_20);
static sat::BlockPnSnClass block_21(21, 21, 3, 9, 6, &page_21);
static sat::BlockPnSnClass block_22(22, 22, 7, 8, 6, &page_22);
static sat::BlockP1SnClass block_23(23, 23, 1, 11, 5, &page_19);
static sat::BlockPnSnClass block_24(24, 24, 5, 9, 5, &page_20);
static sat::BlockPnSnClass block_25(25, 25, 3, 10, 5, &page_21);
static sat::BlockPnSnClass block_26(26, 26, 7, 9, 5, &page_22);
static sat::BlockP1SnClass block_27(27, 27, 1, 12, 4, &page_19);
static sat::BlockPnSnClass block_28(28, 28, 5, 10, 4, &page_20);
static sat::BlockPnSnClass block_29(29, 29, 3, 11, 4, &page_21);
static sat::BlockPnSnClass block_30(30, 30, 7, 10, 4, &page_22);
static sat::BlockP1SnClass block_31(31, 31, 1, 13, 3, &page_19);
static sat::BlockPnSnClass block_32(32, 32, 5, 11, 3, &page_20);
static sat::BlockPnSnClass block_33(33, 33, 3, 12, 3, &page_21);
static sat::BlockPnSnClass block_34(34, 34, 7, 11, 3, &page_22);
static sat::BlockP1SnClass block_35(35, 35, 1, 14, 2, &page_19);
static sat::BlockPnSnClass block_36(36, 36, 5, 12, 2, &page_20);
static sat::BlockPnSnClass block_37(37, 37, 3, 13, 2, &page_21);
static sat::BlockPnSnClass block_38(38, 38, 7, 12, 2, &page_22);
static sat::BlockP1SnClass block_39(39, 39, 1, 15, 1, &page_19);
static sat::BlockPnSnClass block_40(40, 40, 5, 13, 1, &page_20);
static sat::BlockPnSnClass block_41(41, 41, 3, 14, 1, &page_21);
static sat::BlockPnSnClass block_42(42, 42, 7, 13, 1, &page_22);
static sat::BlockSubunitSpanClass block_43(43, 1, 16);
static sat::BlockPnSnClass block_44(44, 43, 5, 14, 0, &page_20);
static sat::BlockPnSnClass block_45(45, 44, 3, 15, 0, &page_21);
static sat::BlockPnSnClass block_46(46, 45, 7, 14, 0, &page_22);
static sat::BlockSubunitSpanClass block_47(47, 1, 17);
static sat::BlockP1SnClass block_48(48, 46, 5, 15, 1, &page_23);
static sat::BlockSubunitSpanClass block_49(49, 3, 16);
static sat::BlockP1SnClass block_50(50, 47, 7, 15, 1, &page_24);
static sat::BlockSubunitSpanClass block_51(51, 1, 18);
static sat::BlockSubunitSpanClass block_52(52, 5, 16);
static sat::BlockSubunitSpanClass block_53(53, 3, 17);
static sat::BlockSubunitSpanClass block_54(54, 7, 16);
static sat::BlockSubunitSpanClass block_55(55, 1, 19);
static sat::BlockSubunitSpanClass block_56(56, 5, 17);
static sat::BlockSubunitSpanClass block_57(57, 3, 18);
static sat::BlockSubunitSpanClass block_58(58, 7, 17);
static sat::BlockSubunitSpanClass block_59(59, 1, 20);
static sat::BlockSubunitSpanClass block_60(60, 5, 18);
static sat::BlockSubunitSpanClass block_61(61, 3, 19);
static sat::BlockSubunitSpanClass block_62(62, 7, 18);
static sat::BlockSubunitSpanClass block_63(63, 1, 21);
static sat::BlockSubunitSpanClass block_64(64, 5, 19);
static sat::BlockSubunitSpanClass block_65(65, 3, 20);
static sat::BlockSubunitSpanClass block_66(66, 7, 19);
static sat::BlockUnitSpanClass block_67(67);
static sat::BlockSubunitSpanClass block_68(68, 5, 20);
static sat::BlockSubunitSpanClass block_69(69, 3, 21);
static sat::BlockSubunitSpanClass block_70(70, 7, 20);
static sat::BlockSubunitSpanClass block_71(71, 5, 21);
static sat::BlockSubunitSpanClass block_72(72, 7, 21);

sat::BlockClass* sat::cBlockClassTable[73] = {
   &block_0, &block_1, &block_2, &block_3, &block_4, &block_5, &block_6, &block_7,
   &block_8, &block_9, &block_10, &block_11, &block_12, &block_13, &block_14, &block_15,
   &block_16, &block_17, &block_18, &block_19, &block_20, &block_21, &block_22, &block_23,
   &block_24, &block_25, &block_26, &block_27, &block_28, &block_29, &block_30, &block_31,
   &block_32, &block_33, &block_34, &block_35, &block_36, &block_37, &block_38, &block_39,
   &block_40, &block_41, &block_42, &block_43, &block_44, &block_45, &block_46, &block_47,
   &block_48, &block_49, &block_50, &block_51, &block_52, &block_53, &block_54, &block_55,
   &block_56, &block_57, &block_58, &block_59, &block_60, &block_61, &block_62, &block_63,
   &block_64, &block_65, &block_66, &block_67, &block_68, &block_69, &block_70, &block_71,
   &block_72, 
};

BlockClass* sat::getBlockClass(size_target_t target) {

    static uint8_t block_sizes_map[24][4] = {
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 1, 1 },
        { 0, 1, 2, 3 },
        { 0, 2, 4, 6 },
        { 1, 5, 8, 10 },
        { 3, 9, 12, 14 },
        { 7, 13, 16, 18 },
        { 11, 17, 20, 22 },
        { 15, 21, 24, 26 },
        { 19, 25, 28, 30 },
        { 23, 29, 32, 34 },
        { 27, 33, 36, 38 },
        { 31, 37, 40, 42 },
        { 35, 41, 44, 46 },
        { 39, 45, 48, 50 },
        { 43, 49, 52, 54 },
        { 47, 53, 56, 58 },
        { 51, 57, 60, 62 },
        { 55, 61, 64, 66 },
        { 59, 65, 68, 70 },
        { 63, 69, 71, 72 },
        { 67, 67, 67, 67 },
        { 71, 67, 67, 67 }
    };
    
    auto id = block_sizes_map[target.shift][target.packing >> 1];
    return sat::cBlockClassTable[id];
}
