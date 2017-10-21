/**
 * This file implements information about a chunk of data.
 *
 * Copyright 2016 University Corporation for Atmospheric Research. All rights
 * reserved. See the file COPYING in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: ChunkInfo.cpp
 * @author: Steven R. Emmerson
 */

#include "ChunkInfo.h"
#include "ProdInfo.h"
#include "error.h"

#include <arpa/inet.h>

namespace hycast {

// Arbitrary
static ChunkSize canonSize = 32768 - 8;

ChunkInfo::ChunkInfo(
        const ProdIndex   prodIndex,
        const ProdSize    prodSize,
        const ChunkIndex  chunkIndex)
    : prodIndex(prodIndex)
    , prodSize(prodSize)
    , chunkIndex(chunkIndex)
    , hashCode{0}
{
    auto numChunks = (prodSize+getCanonSize()-1)/getCanonSize();
    if (chunkIndex && chunkIndex >= numChunks)
        throw INVALID_ARGUMENT("Chunk-index is greater than or equal to number "
                "of chunks: index=" + std::to_string(chunkIndex) +
                ", numChunks=" + std::to_string(numChunks));
}

const ChunkInfo& ChunkInfo::operator =(const ChunkInfo& rhs) noexcept
{
    prodIndex = rhs.prodIndex;
    prodSize = rhs.prodSize;
    chunkIndex = rhs.chunkIndex;
    hashCode = rhs.hashCode.load();
    return *this;
}

ChunkInfo::operator bool() const noexcept
{
    return prodSize != 0 && canonSize != 0;
}

bool ChunkInfo::isEarlierThan(const ChunkInfo& rhs) const noexcept
{
    return prodIndex < rhs.prodIndex ||
            (prodIndex == rhs.prodIndex && chunkIndex < rhs.chunkIndex);
}

void ChunkInfo::setCanonSize(const ChunkSize size)
{
    if (size == 0)
        throw INVALID_ARGUMENT("Cannot set canonical chunk size to zero");
    canonSize = size;
}

ChunkSize ChunkInfo::getCanonSize()
{
    return canonSize;
}

ChunkSize ChunkInfo::getSize(
        const ProdSize   prodSize,
        const ChunkIndex chunkIndex)
{
    const auto offset = getOffset(chunkIndex);
    if (offset >= prodSize)
        throw InvalidArgument(__FILE__, __LINE__,
                "Chunk-offset is greater than or equal to product-size: "
                "offset=" + std::to_string(offset) + ", size=" +
                std::to_string(prodSize) + ", chunkIndex=" +
                std::to_string(chunkIndex));
    auto remaining = prodSize - offset;
    auto canonSize = getCanonSize();
    return canonSize < remaining
            ? canonSize
            : remaining;
}

ChunkInfo::ChunkInfo(
        const ProdInfo&   prodInfo,
        const ChunkIndex  chunkIndex) noexcept
    : ChunkInfo{prodInfo.getIndex(), prodInfo.getSize(), chunkIndex}
{}

ChunkInfo::ChunkInfo(
        Decoder&       decoder,
        const unsigned version)
    : ChunkInfo()
{
    // Keep consonant with serialize()
    prodIndex = ProdIndex::deserialize(decoder, version);
    decoder.decode(prodSize);
    decoder.decode(chunkIndex);
}

bool ChunkInfo::operator==(const ChunkInfo& that) const noexcept
{
    return prodIndex == that.prodIndex && prodSize == that.prodSize &&
            chunkIndex == that.chunkIndex;
}

size_t ChunkInfo::serialize(
        Encoder&       encoder,
        const unsigned version) const
{
    // Keep consonant with ChunkInfo(Decoder, unsigned)
    return encoder.encode(prodIndex) + encoder.encode(prodSize) +
            encoder.encode(chunkIndex);
}

ChunkInfo ChunkInfo::deserialize(
        Decoder&          decoder,
        const unsigned    version)
{
    ChunkInfo chunkInfo(decoder, version);
    return ChunkInfo(chunkInfo.prodIndex, chunkInfo.prodSize,
            chunkInfo.chunkIndex);
}

std::string ChunkInfo::to_string() const
{
    return "{prodIndex=" + prodIndex.to_string() + ", chunkIndex=" +
            std::to_string(chunkIndex) + ", size=" + std::to_string(getSize()) +
            "}";
}

} // namespace
