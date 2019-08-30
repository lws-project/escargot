/*
 * Copyright (C) 2012-2016 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#pragma once

#include <stdint.h>
#ifdef ENABLE_ICU
#include <unicode/utypes.h>
#endif

namespace JSC { namespace Yarr {

// This set of data provides information for each UCS2 code point as to the set of code points
// that it should match under the ES6 case insensitive RegExp matching rules, specified in 21.2.2.8.2.
// The non-Unicode tables are autogenerated using YarrCanonicalize.js into YarrCanonicalize.cpp.
// The Unicode tables are autogenerated using the python script generateYarrCanonicalizeUnicode
// which creates YarrCanonicalizeUnicode.cpp.
enum UCS2CanonicalizationType {
    CanonicalizeUnique,               // No canonically equal values, e.g. 0x0.
    CanonicalizeSet,                  // Value indicates a set in characterSetInfo.
    CanonicalizeRangeLo,              // Value is positive delta to pair, E.g. 0x41 has value 0x20, -> 0x61.
    CanonicalizeRangeHi,              // Value is positive delta to pair, E.g. 0x61 has value 0x20, -> 0x41.
    CanonicalizeAlternatingAligned,   // Aligned consequtive pair, e.g. 0x1f4,0x1f5.
    CanonicalizeAlternatingUnaligned, // Unaligned consequtive pair, e.g. 0x241,0x242.
};
struct CanonicalizationRange {
    UChar32 begin;
    UChar32 end;
    UChar32 value;
    UCS2CanonicalizationType type;
};

extern const size_t UCS2_CANONICALIZATION_RANGES;
extern const UChar32* const ucs2CharacterSetInfo[];
extern const CanonicalizationRange ucs2RangeInfo[];

extern const size_t UNICODE_CANONICALIZATION_RANGES;
extern const UChar32* const unicodeCharacterSetInfo[];
extern const CanonicalizationRange unicodeRangeInfo[];

enum class CanonicalMode { UCS2, Unicode };

inline const UChar32* canonicalCharacterSetInfo(unsigned index, CanonicalMode canonicalMode)
{
    const UChar32* const* rangeInfo = canonicalMode == CanonicalMode::UCS2 ? ucs2CharacterSetInfo : unicodeCharacterSetInfo;
    return rangeInfo[index];
}

// This searches in log2 time over ~400-600 entries, so should typically result in 9 compares.
inline const CanonicalizationRange* canonicalRangeInfoFor(UChar32 ch, CanonicalMode canonicalMode = CanonicalMode::UCS2)
{
    const CanonicalizationRange* info = canonicalMode == CanonicalMode::UCS2 ? ucs2RangeInfo : unicodeRangeInfo;
    size_t entries = canonicalMode == CanonicalMode::UCS2 ? UCS2_CANONICALIZATION_RANGES : UNICODE_CANONICALIZATION_RANGES;

    while (true) {
        size_t candidate = entries >> 1;
        const CanonicalizationRange* candidateInfo = info + candidate;
        if (ch < candidateInfo->begin)
            entries = candidate;
        else if (ch <= candidateInfo->end)
            return candidateInfo;
        else {
            info = candidateInfo + 1;
            entries -= (candidate + 1);
        }
    }
}

// Should only be called for characters that have one canonically matching value.
inline UChar32 getCanonicalPair(const CanonicalizationRange* info, UChar32 ch)
{
    ASSERT(ch >= info->begin && ch <= info->end);
    switch (info->type) {
    case CanonicalizeRangeLo:
        return ch + info->value;
    case CanonicalizeRangeHi:
        return ch - info->value;
    case CanonicalizeAlternatingAligned:
        return ch ^ 1;
    case CanonicalizeAlternatingUnaligned:
        return ((ch - 1) ^ 1) + 1;
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
    RELEASE_ASSERT_NOT_REACHED();
    return 0;
}

// Returns true if no other UCS2 codepoint can match this value.
inline bool isCanonicallyUnique(UChar32 ch, CanonicalMode canonicalMode = CanonicalMode::UCS2)
{
    return canonicalRangeInfoFor(ch, canonicalMode)->type == CanonicalizeUnique;
}

// Returns true if values are equal, under the canonicalization rules.
inline bool areCanonicallyEquivalent(UChar32 a, UChar32 b, CanonicalMode canonicalMode = CanonicalMode::UCS2)
{
    const CanonicalizationRange* info = canonicalRangeInfoFor(a, canonicalMode);
    switch (info->type) {
    case CanonicalizeUnique:
        return a == b;
    case CanonicalizeSet: {
        for (const UChar32* set = canonicalCharacterSetInfo(info->value, canonicalMode); (a = *set); ++set) {
            if (a == b)
                return true;
        }
        return false;
    }
    case CanonicalizeRangeLo:
        return (a == b) || (a + info->value == b);
    case CanonicalizeRangeHi:
        return (a == b) || (a - info->value == b);
    case CanonicalizeAlternatingAligned:
        return (a | 1) == (b | 1);
    case CanonicalizeAlternatingUnaligned:
        return ((a - 1) | 1) == ((b - 1) | 1);
    }

    RELEASE_ASSERT_NOT_REACHED();
    return false;
}

} } // JSC::Yarr
