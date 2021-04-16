/*
 *      N-gram generator.
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

/* $Id: ngram.h 84 2010-02-22 12:37:32Z naoaki $ */

#ifndef __NGRAM_H__
#define __NGRAM_H__

#include <map>
#include <sstream>
#include <string>

namespace simstring
{

/**
 * Obtain a set of letter n-grams in a string.
 *  @param  str     The string.
 *  @param  ins     The insert iterator that receives the set of n-grams.
 *  @param  n       The unit of n-grams.
 *  @param  be      \c true to generate n-grams that encode begin and end of
 *                  a string.
 */
template <
    class string_type,
    class insert_iterator
    >
static void
ngrams(
    const string_type& str,
    insert_iterator ins,
    int n,
    bool be
    )
{
    typedef typename string_type::value_type char_type;
    typedef std::basic_stringstream<char_type> stringstream_type;
    typedef std::map<string_type, int> ngram_stat_type;
    const char_type mark = (char_type)0x01;

    string_type src;
    if (be) {
        // Append marks for begin/end of the string.
        for (int i = 0;i < n-1;++i) src += mark;
        src += str;
        for (int i = 0;i < n-1;++i) src += mark;
    } else if ((int)str.length() < n) {
        // Pad marks when the string is shorter than n.
        src = str;
        for (int i = 0;i < n - (int)str.length();++i) {
            src += mark;
        }
    } else {
        src = str;
    }
    
    // Count n-grams in the string.
    ngram_stat_type stat;
    for (typename string_type::size_type i = 0;i < src.length()-n+1;++i) {
        string_type ngram = src.substr(i, n);
        ++stat[ngram];
    }

    // Convert the n-gram stat into a set.
    typename ngram_stat_type::const_iterator it;
    for (it = stat.begin();it != stat.end();++it) {
        *ins = it->first;
        // Append numbers if the same n-gram occurs more than once.
        for (int i = 2;i <= it->second;++i) {
            stringstream_type ss;
            ss << it->first << i;
            *ins = ss.str();
        }
    }
}

/**
 * N-gram generator.
 *
 *  This class generates n-grams for a string.
 */
class ngram_generator
{
protected:
    int m_n;            ///< The unit of n-grams.
    bool m_be;          ///< The flag for begin/end of tokens.

public:
    /**
     * Constructs an instance as a tri-gram generator.
     */
    ngram_generator() : m_n(3), m_be(false)
    {
    }

    /**
     * Constructs an instance as an n-gram generator.
     *  @param  n       The unit of n-grams.
     *  @param  be      \c true to generate n-grams that encode begin and
     *                  end of a string.
     */
    ngram_generator(int n, bool be=false) : m_n(n), m_be(be)
    {
    }

    /**
     * Sets the parameters for n-gram generation.
     *  @param  n       The unit of n-grams.
     *  @param  be      \c true to generate n-grams that encode begin and
     *                  end of a string.
     */
    void set(int n, bool be=false)
    {
        m_n = n;
        m_be = be;
    }

    /**
     * Gets the unit of n-grams.
     *  @return int     The unit of n-grams.
     */
    int get_n() const
    {
        return m_n;
    }

    /**
     * Gets the flag for representing a begin/end of letters.
     *  @return bool    \c true if n-grams encoding the begin and end of a
     *                  string are generated.
     */
    bool get_be() const
    {
        return m_be;
    }

    /**
     * Obtain a set of letter n-grams in a string.
     *  @param  str     The string.
     *  @param  ins     The insert iterator that receives the set of n-grams.
     */
    template <class string_type, class insert_iterator>
    void operator()(const string_type& str, insert_iterator ins) const
    {
        ngrams(str, ins, m_n, m_be);
    }
};

};

#endif/*__NGRAM_H__*/
