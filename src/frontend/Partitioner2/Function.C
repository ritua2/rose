#include "sage3basic.h"
#include <Partitioner2/Function.h>

#include <Partitioner2/Exception.h>
#include <Partitioner2/Utility.h>
#include <integerOps.h>

namespace rose {
namespace BinaryAnalysis {
namespace Partitioner2 {

const std::string&
Function::demangledName() const {
    return demangledName_.empty() ? name_ : demangledName_;
}

bool
Function::insertDataBlock(const DataBlock::Ptr &dblock) {
    if (isFrozen_)
        throw Exception(printableName() + " is frozen or attached to the CFG/AUM");
    if (dblock==NULL)
        return false;
    bool retval = insertUnique(dblocks_, dblock, sortDataBlocks);
    if (retval)
        clearCache();
    return retval;
}

void
Function::eraseDataBlock(const DataBlock::Ptr &dblock) {
    if (dblock) {
        std::vector<DataBlock::Ptr>::iterator lb = std::lower_bound(dblocks_.begin(), dblocks_.end(), dblock, sortDataBlocks);
        if (lb!=dblocks_.end() && (*lb)==dblock) {
            dblocks_.erase(lb);
            clearCache();
        }
    }
}

std::string
Function::printableName() const {
    std::string s = "function " + StringUtility::addrToString(address());
    const std::string &name = demangledName();
    if (!name.empty())
        s += " \"" + StringUtility::cEscape(name) + "\"";
    return s;
}

bool
Function::isThunk() const {
    if (0 == (reasons_ & SgAsmFunction::FUNC_THUNK))
        return false;
    if (bblockVas_.size() != 1)
        return false;
    return true;
}

InstructionSemantics2::BaseSemantics::SValuePtr
Function::stackDeltaOverride() const {
    return stackDeltaOverride_;
}

void
Function::stackDeltaOverride(const InstructionSemantics2::BaseSemantics::SValuePtr &delta) {
    stackDeltaOverride_ = delta;
}

InstructionSemantics2::BaseSemantics::SValuePtr
Function::stackDelta() const {
    if (stackDeltaOverride_ != NULL)
        return stackDeltaOverride_;
    return stackDeltaAnalysis_.functionStackDelta();
}

int64_t
Function::stackDeltaConcrete() const {
    BaseSemantics::SValuePtr v = stackDelta();
    if (v && v->is_number() && v->get_width() <= 64)
        return IntegerOps::signExtend2<uint64_t>(v->get_number(), v->get_width(), 64);
    return SgAsmInstruction::INVALID_STACK_DELTA;
}

} // namespace
} // namespace
} // namespace
