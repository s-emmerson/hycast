/**
 * This file declares the implementation of a data-product.
 *
 * Copyright 2016 University Corporation for Atmospheric Research. All rights
 * reserved. See the file COPYING in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: product.h
 * @author: Steven R. Emmerson
 */

#ifndef PRODUCTIMPL_H_
#define PRODUCTIMPL_H_

#include "Chunk.h"
#include "ProdInfo.h"

#include <vector>

namespace hycast {

class ProductImpl final {
    ProdInfo          prodInfo;
    std::vector<bool> haveChunk;
    char*             data;
    ChunkIndex        numChunks;

    /**
     * Returns a pointer to the start of a chunk-of-data in the accumulating
     * buffer.
     * @param chunkIndex  Index of the chunk
     * @return            Pointer to the start of the chunk
     */
    char* startOf(const ChunkIndex chunkIndex) const;
public:
    /**
     * Constructs from information on a product.
     * @param[in] info Information on a product
     */
    explicit ProductImpl(const ProdInfo& info);
    /**
     * Returns information on the product.
     * @return Information on the product
     * @exceptionsafety Nothrow
     * @threadsafety    Safe
     */
    const ProdInfo& getInfo() const noexcept;
    /**
     * Destroys this instance.
     */
    ~ProductImpl();
    /**
     * Adds a chunk-of-data.
     * @param[in] chunk  The chunk
     * @return `true`    if the chunk of data was added
     * @return `false`   if the chunk of data had already been added. The
     *                   instance is unchanged.
     * @throws std::invalid_argument if the chunk is inconsistent with this
     *                               instance
     * @execptionsafety  Strong guarantee
     * @threadsafety     Compatible but not safe
     */
    bool add(const ActualChunk& chunk);
    /**
     * Adds a latent chunk-of-data.
     * @param[in] chunk  The latent chunk
     * @return `true`    if the chunk of data was added
     * @return `false`   if the chunk of data had already been added. The
     *                   instance is unchanged.
     * @throws std::invalid_argument if the chunk is inconsistent with this
     *                               instance
     * @throws std::system_error     if an I/O error occurs
     * @execptionsafety  Strong guarantee
     * @threadsafety     Compatible but not safe
     */
    bool add(LatentChunk& chunk);
    /**
     * Indicates if this instance is complete (i.e., contains all
     * chunks-of-data).
     * @return `true` iff this instance is complete
     */
    bool isComplete() const;
    /**
     * Returns a pointer to the data.
     * @return a pointer to the data
     * @exceptionsafety Nothrow
     * @threadsafety    Safe
     */
    const char* getData() const noexcept;
};

} // namespace

#endif /* PRODUCTIMPL_H_ */