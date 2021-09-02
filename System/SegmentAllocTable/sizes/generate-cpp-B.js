"use strict"
const fs = require("fs");
const { getMinBits, readCsvFile } = require("./common")

class PageClass {
    id = -1
    binID = -1
    paging = { offset: 0, scale: 0 }
    constructor(model, data) {
        Object.assign(this, data)
        this.model = model
    }
    setup() { }
    equals(other) { return false }
    print() { console.log(this.prototype.name) }
    getDecl() { return `decl_not_found_${this.getName()};` }
    getName() { return `page_${this.id}` }
}

class BlockClass {
    id = -1
    binID = -1
    constructor(model) {
        this.model = model
        this.paging = 0
    }
    setup() { }
    equals(other) { return false }
    print() { console.log(this.prototype.name) }
    getDecl() { return `decl_not_found_${this.getName()};` }
    getName() { return `block_${this.id}` }
}

function NumberToFixedPoint32(value) {
    return Math.min(Math.trunc(Math.pow(2, 32) * value) + 1, 4294967295)
}

/*****************************************************
*
*  PnS1 Class:
*  - used for small block < 1024 bytes
*  - use page batch of 1 segment
*  - pages have 64 blocks, excepted the last one which is truncated
*
******************************************************/
class PagePnS1Class extends PageClass {
    constructor(model, packing, shift) {
        super(model)
        this.packing = packing
        this.shift = shift
        this.page_size = this.packing * Math.pow(2, this.shift)
    }
    setup() {
        this.binID = this.model.createPageBinID()
        this.pagingID = this.model.addPaging(0, NumberToFixedPoint32(1 / this.page_size))
    }
    getDecl() {
        return `static sat::PagePnS1Class ${this.getName()}(${this.id}, ${this.binID}, ${this.pagingID}, ${this.packing}, ${this.shift});`
    }
}
class BlockPnS1Class extends BlockClass {
    constructor(model, data) {
        super(model, data)
        this.packing = data.packing
        this.shift = getMinBits(data.base)
        this.block_per_page_L2 = getMinBits(data.page_length)
        this.page = model.addPageClass(new PagePnS1Class(model, this.packing, this.shift + this.block_per_page_L2))
        console.assert(data.page_length === 64)
    }
    setup() {
        this.binID = this.model.createBlockBinID()
    }
    getDecl() {
        return `static sat::BlockPnS1Class ${this.getName()}(${this.id}, ${this.binID}, ${this.packing}, ${this.shift}, &${this.page.getName()});`
    }
}


/*****************************************************
*
*  PnSn Class:
*  - used for medium block >= 1024 and < 114k bytes
*  - use n page on n segment
*
******************************************************/
class PagePnSnClass extends PageClass {
    pagingIDs = []
    constructor(model, packing, shift) {
        super(model)
        this.packing = packing
        this.shift = shift
        this.page_size = this.packing * Math.pow(2, this.shift)
    }
    setup() {
        this.binID = this.model.createPageBinID()
        this.pagingIDs = []
        for (let i = 0; i < this.packing; i++) {
            this.pagingIDs.push(this.model.addPaging(65536 * i, NumberToFixedPoint32(1 / this.page_size)))
        }
    }
    equals(other) {
        if (!(other instanceof PagePnSnClass)) return false
        if (other.packing != this.packing) return false
        if (other.shift != this.shift) return false
        return true
    }
    getDecl() {
        return `static sat::PagePnSnClass ${this.getName()}(${this.id}, ${this.binID}, ${this.packing}, ${this.shift}, {${this.pagingIDs.join(",")}});`
    }
}
class BlockPnSnClass extends BlockClass {
    constructor(model, data) {
        super(model, data)
        this.packing = data.packing
        this.shift = getMinBits(data.base)
        this.block_per_page_L2 = getMinBits(data.page_length)
        this.page = model.addPageClass(new PagePnSnClass(model, this.packing, this.shift + this.block_per_page_L2))
        console.assert(data.page_length === Math.pow(2, this.block_per_page_L2))
    }
    setup() {
        this.binID = this.model.createBlockBinID()
    }
    equals(other) {
        if (!(other instanceof BlockPnSnClass)) return false
        if (other.packing != this.packing) return false
        if (other.shift != this.shift) return false
        return true
    }
    getDecl() {
        return `static sat::BlockPnSnClass ${this.getName()}(${this.id}, ${this.binID}, ${this.packing}, ${this.shift}, ${this.block_per_page_L2}, &${this.page.getName()});`
    }
}



/*****************************************************
*
*  P1Sn Class
*  - used for medium block >= 1024 and < 229k bytes
*  - use one page on n segment
*
******************************************************/
class PageP1SnClass extends PageClass {
    pagingIDs = []
    constructor(model, packing, shift) {
        super(model)
        this.packing = packing
        this.shift = shift
        this.page_size = this.packing * Math.pow(2, this.shift)
    }
    setup() {
        this.pagingIDs = []
        for (let i = 0; i < this.packing; i++) {
            this.pagingIDs.push(this.model.addPaging(65536 * i, NumberToFixedPoint32(1 / this.page_size)))
        }
    }
    equals(other) {
        if (!(other instanceof PageP1SnClass)) return false
        if (other.packing != this.packing) return false
        if (other.shift != this.shift) return false
        return true
    }
    getDecl() {
        return `static sat::PageP1SnClass ${this.getName()}(${this.id}, ${this.packing}, ${this.shift}, {${this.pagingIDs.join(",")}});`
    }
}
class BlockP1SnClass extends BlockClass {
    constructor(model, data) {
        super(model, data)
        this.packing = data.packing
        this.shift = getMinBits(data.base)
        this.block_per_page_L2 = getMinBits(data.page_length)
        this.page = model.addPageClass(new PageP1SnClass(model, this.packing, this.shift + this.block_per_page_L2))
    }
    setup() {
        this.binID = this.model.createBlockBinID()
    }
    getDecl() {
        return `static sat::BlockP1SnClass ${this.getName()}(${this.id}, ${this.binID}, ${this.packing}, ${this.shift}, ${this.block_per_page_L2}, &${this.page.getName()});`
    }
}


/*****************************************************
*
*  Subunit-Span Class:
*  - used for large block >= 64k and < 14M bytes
*  - use one page on n subunit
*
******************************************************/
class BlockSubunitSpanClass extends BlockClass {
    constructor(model, data) {
        super(model, data)
        this.packing = data.packing
        this.shift = getMinBits(data.base)
    }
    getDecl() {
        return `static sat::BlockSubunitSpanClass ${this.getName()}(${this.id}, ${this.packing}, ${this.shift});`
    }
}


/*****************************************************
*
*  Unit-Span Class:
*  - used for huge block >= 4Mb and < 4Gb bytes
*  - use one page on n unit
*
******************************************************/
class BlockUnitSpanClass extends BlockClass {
    equals(other) {
        return other instanceof BlockUnitSpanClass
    }
    getDecl() {
        return `static sat::BlockUnitSpanClass ${this.getName()}(${this.id});`
    }
}


class MemoryModel {
    sizeClasses = []

    blockClasses = []
    blockBinCount = 0

    pageClasses = []
    pageBinCount = 0

    pagings = [{ offset: 0, scale: 0 }]

    constructor() {
        this.sizeClasses = readCsvFile("./size-classes-B.csv").filter(entry => !!entry.algo)
        this.sizeClasses.push({ algo: "unit", size: Math.pow(2, 32) })
        this.sizeClasses.sort((x, y) => x.size - y.size)
        for (const cls of this.sizeClasses) {
            cls.shift = getMinBits(cls.base)
            cls.$block = this.createBlockClass(cls)
        }
        this.setup()
    }
    getSizeBestBlock(size) {
        for (const cls of this.sizeClasses) {
            if (cls.size >= size) return cls.$block
        }
        throw new Error()
    }
    createBlockBinID() {
        return this.blockBinCount++
    }
    addBlockClass(cls) {
        const existing = this.blockClasses.find(x => x.equals(cls))
        if (existing) return existing
        this.blockClasses.push(cls)
        return cls
    }
    createBlockClass(data) {
        function create(model) {
            switch (data.algo) {
                case 'PnS1': return new BlockPnS1Class(model, data)
                case 'PnSn': return new BlockPnSnClass(model, data)
                case 'P1Sn': return new BlockP1SnClass(model, data)
                case 'sunit': return new BlockSubunitSpanClass(model, data)
                case 'unit': return new BlockUnitSpanClass(model, data)
                default: throw new Error(`unkown block algo '${data.algo}'`)
            }
        }
        return this.addBlockClass(create(this))
    }

    createPageBinID() {
        return this.pageBinCount++
    }
    addPageClass(cls) {
        const existing = this.pageClasses.find(x => x.equals(cls))
        if (existing) return existing
        this.pageClasses.push(cls)
        return cls
    }
    addPaging(offset, scale) {
        let index = this.pagings.findIndex(x => x.offset === offset && x.scale === scale)
        if (index < 0) {
            index = this.pagings.length
            this.pagings.push({ offset, scale })
        }
        return index
    }
    setup() {

        let bid = 0
        for (const blk of this.blockClasses) blk.id = bid++
        for (const blk of this.blockClasses) blk.setup()

        let pid = 0
        for (const pg of this.pageClasses) pg.id = pid++
        for (const pg of this.pageClasses) pg.setup()
    }
}

function generateCpp() {
    const model = new MemoryModel()

    const block_declarations = []
    const block_class_table = []
    for (const blk of model.blockClasses) {
        block_declarations.push(blk.getDecl())
        block_class_table.push(blk.getName())
    }

    const page_declarations = []
    const page_class_table = []
    for (const pg of model.pageClasses) {
        page_declarations.push(pg.getDecl())
        page_class_table.push(pg.getName())
    }

    const code_table = []
    for (let shift = 0; shift < 24; shift++) {
        const line = []
        for (let packing = 1; packing <= 7; packing += 2) {
            const block = model.getSizeBestBlock(packing * Math.pow(2, shift))
            line.push(`${block.id}`)
        }
        code_table.push(`{ ${line.join(", ")} }`)
    }

    // Generate c++ header
    const file_h = `#pragma once
#include "./memory-context.h"

namespace sat {

    struct SegmentPaging;
    struct BlockClass;
    struct PageClass;
 
    static const int cSegmentPagingsCount = ${model.pagings.length};
    extern const SegmentPaging cSegmentPagings[${model.pagings.length}];

    static const int cBlockBinCount = ${model.blockBinCount};
    static const int cBlockClassCount = ${model.blockClasses.length};
    extern BlockClass* cBlockClassTable[${model.blockClasses.length}];

    static const int cPageBinCount = ${model.pageBinCount};
    static const int cPageClassCount = ${model.pageClasses.length};
    extern PageClass* cPageClassTable[${model.pageClasses.length}];

    BlockClass* getBlockClass(size_target_t target);
}
`
    const file_cpp = `
#include "./blocks.h"
#include "./blocks-classes.h"

using namespace sat;

const sat::SegmentPaging sat::cSegmentPagings[${model.pagings.length}] = {
    ${model.pagings.map(x => `{ ${x.offset}, ${x.scale} }`).join(",\n    ")}
};

${page_declarations.join("\n")}

sat::PageClass* sat::cPageClassTable[${page_class_table.length}] = {
   ${page_class_table.reduce((txt, item, i) => txt + "&" + item + ((i + 1) % 8 ? ", " : ",\n   "), "")}
};

${block_declarations.join("\n")}

sat::BlockClass* sat::cBlockClassTable[${block_class_table.length}] = {
   ${block_class_table.reduce((txt, item, i) => txt + "&" + item + ((i + 1) % 8 ? ", " : ",\n   "), "")}
};

BlockClass* sat::getBlockClass(size_target_t target) {

    static uint8_t block_sizes_map[${code_table.length}][4] = {
        ${code_table.join(",\n        ")}
    };
    
    auto id = block_sizes_map[target.shift][target.packing >> 1];
    return sat::cBlockClassTable[id];
}
`
    fs.writeFileSync("../src/blocks-classes.h", file_h)
    fs.writeFileSync("../src/blocks-classes.cpp", file_cpp)
}

generateCpp()