/*
 *      SimString similarity measures.
 *
 * Copyright (c) 2009,2010 Naoaki Okazaki
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the authors nor the names of its contributors may
 *       be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* $Id: measure.h 104 2010-03-07 07:43:40Z naoaki $ */


#ifndef __SIMSTRING_MEASURE_H__
#define __SIMSTRING_MEASURE_H__

#include <limits.h>

namespace simstring { namespace measure {

/**
 * This class implements the traits of exact matching.
 */
struct exact
{
    inline static int min_size(int qsize, double alpha)
    {
        return qsize;
    }

    inline static int max_size(int qsize, double alpha)
    {
        return qsize;
    }

    inline static int min_match(int qsize, int rsize, double alpha)
    {
        return qsize;
    }
};

/**
 * This class implements the traits of dice coefficient.
 */
struct dice
{
    inline static int min_size(int qsize, double alpha)
    {
        return (int)std::ceil(alpha * qsize / (2. - qsize));
    }

    inline static int max_size(int qsize, double alpha)
    {
        return (int)std::floor((2. - alpha) * qsize / alpha);
    }

    inline static int min_match(int qsize, int rsize, double alpha)
    {
        return (int)std::ceil(0.5 * alpha * (qsize + rsize));
    }
};

/**
 * This class implements the traits of cosine coefficient.
 */
struct cosine
{
    inline static int min_size(int qsize, double alpha)
    {
        return (int)std::ceil(alpha * alpha * qsize);
    }

    inline static int max_size(int qsize, double alpha)
    {
        return (int)std::floor(qsize / (alpha * alpha));
    }

    inline static int min_match(int qsize, int rsize, double alpha)
    {
        return (int)std::ceil(alpha * std::sqrt((double)qsize * rsize));
    }
};

/**
 * This class implements the traits of Jaccard coefficient.
 */
struct jaccard
{
    inline static int min_size(int qsize, double alpha)
    {
        return (int)std::ceil(alpha * qsize);
    }

    inline static int max_size(int qsize, double alpha)
    {
        return (int)std::floor(qsize / alpha);
    }

    inline static int min_match(int qsize, int rsize, double alpha)
    {
        return (int)std::ceil(alpha * (qsize + rsize) / (1 + alpha));
    }
};

/**
 * This class implements the traits of overlap coefficient.
 */
struct overlap
{
    inline static int min_size(int qsize, double alpha)
    {
        return 1;
    }

    inline static int max_size(int qsize, double alpha)
    {
        return (int)INT_MAX;
    }

    inline static int min_match(int qsize, int rsize, double alpha)
    {
        return (int)std::ceil(alpha * std::min(qsize, rsize));
    }
};

}; };

#endif/*__SIMSTRING_MEASURE_H__*/
