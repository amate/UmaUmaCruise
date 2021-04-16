/*
 *      C++ implementation of Constant Database (CDB++)
 *
 * Copyright (c) 2008-2010 Naoaki Okazaki
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

/* $Id: cdbpp.h 98 2010-02-26 05:51:27Z naoaki $ */

#ifndef __CDBPP_H__
#define __CDBPP_H__

#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <vector>
#include <stdint.h>
#include <stdexcept>

namespace cdbpp
{

/** 
 * \addtogroup cdbpp_api CDB++ API
 * @{
 *
 *  The CDB++ API.
 */

// Global constants.
enum {
    // Version number.
    CDBPP_VERSION = 1,
    // The number of hash tables.
    NUM_TABLES = 256,
    // A constant for byte-order checking.
    BYTEORDER_CHECK = 0x62445371,
};




/**
 * MurmurHash2.
 *
 *  This code makes the following assumption about how your machine behaves
 *      - We can read a 4-byte value from any address without crashing.
 *
 *  It also has a few limitations:
 *      - It will not work incrementally.
 *      - It will not produce the same results on little-endian and big-endian
 *        machines.
 *
 *  @author Austin Appleby
 */
class murmurhash2 :
    public std::binary_function<const void *, size_t, uint32_t>
{
protected:
    inline static uint32_t get32bits(const char *d)
    {
        return *reinterpret_cast<const uint32_t*>(d);
    }

public:
    inline uint32_t operator() (const void *key, size_t size) const
    {
        // 'm' and 'r' are mixing constants generated offline.
        // They're not really 'magic', they just happen to work well.

        const uint32_t m = 0x5bd1e995;
        const int32_t r = 24;

        // Initialize the hash to a 'random' value

        const uint32_t seed = 0x87654321;
        uint32_t h = seed ^ size;

        // Mix 4 bytes at a time into the hash

        const char * data = (const char *)key;

        while (size >= 4)
        {
            uint32_t k = get32bits(data);

            k *= m; 
            k ^= k >> r; 
            k *= m; 
            
            h *= m; 
            h ^= k;

            data += 4;
            size -= 4;
        }
        
        // Handle the last few bytes of the input array

        switch (size)
        {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0];
                h *= m;
        };

        // Do a few final mixes of the hash to ensure the last few
        // bytes are well-incorporated.

        h ^= h >> 13;
        h *= m;
        h ^= h >> 15;

        return h;
    } 
};




struct tableref_t
{
    uint32_t    offset;     // Offset to a hash table.
    uint32_t    num;        // Number of elements in the hash table.
};


static uint32_t get_data_begin()
{
    return (16 + sizeof(tableref_t) * NUM_TABLES);
}



/**
 * Exception class for the CDB++ builder.
 */
class builder_exception : public std::invalid_argument
{
public:
    builder_exception(const std::string& msg)
        : std::invalid_argument(msg)
    {
    }
};



/**
 * CDB++ builder.
 */
template <typename hash_function>
class builder_base
{
protected:
    // A bucket structure.
    struct bucket
    {
        uint32_t    hash;       // Hash value of the record.
        uint32_t    offset;     // Offset address to the actual record.

        bucket() : hash(0), offset(0)
        {
        }

        bucket(uint32_t h, uint32_t o) : hash(h), offset(o)
        {
        }
    };

    // A hash table is a vector of buckets.
    typedef std::vector<bucket> hashtable;

protected:
    std::ofstream&  m_os;               // Output stream.
    uint32_t        m_begin;
    uint32_t        m_cur;
    hashtable       m_ht[NUM_TABLES];   // Hash tables.

public:
    /**
     * Constructs an object.
     *  @param  os          The output stream to which this class write the
     *                      database. This stream must be opened in the
     *                      binary mode (\c std::ios_base::binary).
     */
    builder_base(std::ofstream& os) : m_os(os)
    {
        m_begin = m_os.tellp();
        m_cur = get_data_begin();
        m_os.seekp(m_begin + m_cur);
    }

    /**
     * Destructs an object.
     */
    virtual ~builder_base()
    {
        this->close();
    }

    /**
     * Inserts a pair of key and value to the database.
     *  Any key in the database should be unique, but this library does not
     *  check duplicated keys.
     *  @param  key         The pointer to the key.
     *  @param  ksize       The size of the key.
     *  @param  value       The pointer to the value.
     *  @param  vsize       The size of the value.
     */
    template <class key_t, class value_t>
    void put(const key_t *key, size_t ksize, const value_t *value, size_t vsize)
    {
        // Write out the current record.
        write_uint32((uint32_t)ksize);
        m_os.write(reinterpret_cast<const char *>(key), ksize);
        write_uint32((uint32_t)vsize);
        m_os.write(reinterpret_cast<const char *>(value), vsize);

        // Compute the hash value and choose a hash table.
        uint32_t hv = hash_function()(static_cast<const void *>(key), ksize);
        hashtable& ht = m_ht[hv % NUM_TABLES];

        // Store the hash value and offset to the hash table.
        ht.push_back(bucket(hv, m_cur));

        // Increment the current position.
        m_cur += sizeof(uint32_t) + ksize + sizeof(uint32_t) + vsize;
    }

protected:
    void close()
    {
        // Check the consistency of the stream offset.
        if (m_begin + m_cur != (uint32_t)m_os.tellp()) {
            throw builder_exception("Inconsistent stream offset");
        }

        // Store the hash tables. At this moment, the file pointer refers to
        // the offset succeeding the last key/value pair.
        for (size_t i = 0;i < NUM_TABLES;++i) {
            hashtable& ht = m_ht[i];

            // Do not write an empty hash table.
            if (!ht.empty()) {
                // An actual table will have the double size; half elements
                // in the table are kept empty.
                int n = ht.size() * 2;

                // Allocate the actual table.
                bucket* dst = new bucket[n];

                // Put hash elements to the table with the open-address method.
                typename hashtable::const_iterator it;
                for (it = ht.begin();it != ht.end();++it) {
                    int k = (it->hash >> 8) % n;

                    // Find a vacant element.
                    while (dst[k].offset != 0) {
                        k = (k+1) % n;
                    }

                    // Store the hash element.
                    dst[k].hash = it->hash;
                    dst[k].offset = it->offset;
                }

                // Write out the new table.
                for (int k = 0;k < n;++k) {
                    write_uint32(dst[k].hash);
                    write_uint32(dst[k].offset);
                }

                // Free the table.
                delete[] dst;
            }
        }

        // Store the current position.
        uint32_t offset = (uint32_t)m_os.tellp();

        // Rewind the stream position to the beginning.
        m_os.seekp(m_begin);

        // Write the file header.
        char chunkid[4] = {'C','D','B','+'};
        m_os.write(chunkid, 4);
        write_uint32(offset - m_begin);
        write_uint32(CDBPP_VERSION);
        write_uint32(BYTEORDER_CHECK);

        // Write references to hash tables. At this moment, dbw->cur points
        // to the offset succeeding the last key/data pair. 
        for (size_t i = 0;i < NUM_TABLES;++i) {
            // Offset to the hash table (or zero for non-existent tables).
            write_uint32(m_ht[i].empty() ? 0 : m_cur);
            // Bucket size is double to the number of elements.
            write_uint32(m_ht[i].size() * 2);
            // Advance the offset counter.
            m_cur += sizeof(uint32_t) * 2 * m_ht[i].size() * 2;
        }

        // Seek to the last position.
        m_os.seekp(offset);
    }

    inline void write_uint32(uint32_t value)
    {
        m_os.write(reinterpret_cast<const char *>(&value), sizeof(value));
    }
};



/**
 * Exception class for the CDB++ reader.
 */
class cdbpp_exception : public std::invalid_argument
{
public:
    cdbpp_exception(const std::string& msg)
        : std::invalid_argument(msg)
    {
    }
};

/**
 * CDB++ reader.
 */
template <typename hash_function>
class cdbpp_base
{
protected:
    struct bucket_t
    {
        uint32_t    hash;           // Hash value of the record.
        uint32_t    offset;         // Offset address to the actual record.
    };


    struct hashtable_t
    {
        uint32_t        num;            // Number of elements in the table.
        const bucket_t* buckets;        // Buckets (array of bucket).
    };


protected:
    const uint8_t*  m_buffer;           // Pointer to the memory block.
    size_t          m_size;             // Size of the memory block.
    bool            m_own;              // 

    hashtable_t     m_ht[NUM_TABLES];   // Hash tables.
    size_t          m_n;

public:
    /**
     * Constructs an object.
     */
    cdbpp_base()
        : m_buffer(NULL), m_size(0), m_own(false), m_n(0)
    {
    }

    /**
     * Constructs an object by opening a database on memory.
     *  @param  buffer      The pointer to the memory image of the database.
     *  @param  size        The size of the memory image.
     *  @param  own         If this is set to \c true, this library will call
     *                      delete[] when the database is closed.
     */
    cdbpp_base(const void *buffer, size_t size, bool own)
        : m_buffer(NULL), m_size(0), m_own(false), m_n(0)
    {
        this->open(buffer, size, own);
    }

    /**
     * Constructs an object by opening a database from an input stream.
     *  @param  ifs         The input stream from which this library reads
     *                      a database.
     */
    cdbpp_base(std::ifstream& ifs)
        : m_buffer(NULL), m_size(0), m_own(false), m_n(0)
    {
        this->open(ifs);
    }

    /**
     * Destructs the object.
     */
    virtual ~cdbpp_base()
    {
        close();
    }

    /**
     * Tests if the database is opened.
     *  @return bool        \c true if the database is opened,
     *                      \c false otherwise.
     */
    bool is_open() const
    {
        return (m_buffer != NULL);
    }

    /**
     * Obtains the number of elements in the database.
     *  @return size_t          The number of elements.
     */
    size_t size() const
    {
        return m_n;
    }

    /**
     * Tests if the database is empty.
     *  @return bool        \c true if the number of records is zero,
     *                      \c false otherwise.
     */
    bool empty() const
    {
        return (m_n == 0);
    }

    /**
     * Opens the database from an input stream.
     *  @param  ifs         The input stream from which this library reads
     *                      a database.
     */
    size_t open(std::ifstream& ifs)
    {
        char chunk[4], size[4];
        std::istream::pos_type offset = ifs.tellg();

        do {
            // Read a chunk identifier.
            ifs.read(chunk, 4);
            if (ifs.fail()) {
                break;
            }

            // Check the chunk identifier.
            if (std::strncmp(chunk, "CDB+", 4) != 0) {
                break;
            }

            // Read the size of the chunk.
            ifs.read(size, 4);
            if (ifs.fail()) {
                break;
            }

            // Allocate a memory block for the chunk.
            uint32_t chunk_size = read_uint32(reinterpret_cast<uint8_t*>(size));
            uint8_t* block = new uint8_t[chunk_size];

            // Read the memory image from the stream.
            ifs.seekg(0, std::ios_base::beg);
            if (ifs.fail()) {
                break;
            }
            ifs.read(reinterpret_cast<char*>(block), chunk_size);
            if (ifs.fail()) {
                break;
            }

            return this->open(block, chunk_size, true);

        } while (0);

        ifs.seekg(offset, std::ios::beg);
        return 0;
    }

    /**
     * Opens the database from a memory image.
     *  @param  buffer      The pointer to the memory image of the database.
     *  @param  size        The size of the memory image.
     *  @param  own         If this is set to \c true, this library will call
     *                      delete[] when the database is closed.
     */
    size_t open(const void *buffer, size_t size, bool own = false)
    {
        const uint8_t *p = reinterpret_cast<const uint8_t*>(buffer);

        // Make sure that the size of the chunk is larger than the minimum size.
        if (size < get_data_begin()) {
            throw cdbpp_exception("The memory image is smaller than a chunk header.");
        }

        // Check the chunk identifier.
        if (memcmp(p, "CDB+", 4) != 0) {
            throw cdbpp_exception("Incorrect chunk header");
        }
        p += 4;

        // Read the chunk header.
        uint32_t csize = read_uint32(p);
        p += sizeof(uint32_t);
        uint32_t version = read_uint32(p);
        p += sizeof(uint32_t);
        uint32_t byteorder = read_uint32(p);
        p += sizeof(uint32_t);

        // Check the byte-order consistency.
        if (byteorder != BYTEORDER_CHECK) {
            throw cdbpp_exception("Inconsistent byte order");
        }
        // Check the version number.
        if (version != CDBPP_VERSION) {
            throw cdbpp_exception("Incompatible CDB++ versions");
        }
        // Check the chunk size.
        if (size < csize) {
            throw cdbpp_exception("The memory image is smaller than a chunk size.");
        }

        // Set memory block and size.
        m_buffer = reinterpret_cast<const uint8_t*>(buffer);
        m_size = size;
        m_own = own;

        // Set pointers to the hash tables.
        m_n = 0;
        const tableref_t* ref = reinterpret_cast<const tableref_t*>(p);
        for (size_t i = 0;i < NUM_TABLES;++i) {
            if (ref[i].offset) {
                // Set the buckets.
                m_ht[i].buckets = reinterpret_cast<const bucket_t*>(m_buffer + ref[i].offset);
                m_ht[i].num = ref[i].num;
            } else {
                // An empty hash table.
                m_ht[i].buckets = NULL;
                m_ht[i].num = 0;
            }

            // The number of records is the half of the table size.
            m_n += (ref[i].num / 2);
        }

        return (size_t)csize;
    }

    /**
     * Closes the database.
     */
    void close()
    {
        if (m_own && m_buffer != NULL) {
            delete[] m_buffer;
        }
        m_buffer = NULL;
        m_size = 0;
        m_n = 0;
    }

    /**
     * Finds the key in the database.
     *  @param  key         The pointer to the key.
     *  @param  ksize       The size of the key.
     *  @param  vsize       The pointer of a variable to which the size of the
     *                      value returned. This parameter can be \c NULL.
     *  @return const void* The pointer to the value.
     */
    const void* get(const void *key, size_t ksize, size_t* vsize) const
    {
        uint32_t hv = hash_function()(key, ksize);
        const hashtable_t* ht = &m_ht[hv % NUM_TABLES];

        if (ht->num && ht->buckets != NULL) {
            int n = ht->num;
            int k = (hv >> 8) % n;
            const bucket_t* p = NULL;

            while (p = &ht->buckets[k], p->offset) {
                if (p->hash == hv) {
                    const uint8_t *q = m_buffer + p->offset;
                    if (read_uint32(q) == ksize &&
                        memcmp(key, q + sizeof(uint32_t), ksize) == 0) {
                        q += sizeof(uint32_t) + ksize;
                        if (vsize != NULL) {
                            *vsize = read_uint32(q);
                        }
                        return q + sizeof(uint32_t);
                    }
                }
                k = (k+1) % n;
            }
        }

        if (vsize != NULL) {
            *vsize = 0;
        }
        return NULL;
    }

protected:
    inline uint32_t read_uint32(const uint8_t* p) const
    {
        return *reinterpret_cast<const uint32_t*>(p);
    }
};

/// CDB++ builder with MurmurHash2.
typedef builder_base<murmurhash2> builder;
/// CDB++ reader with MurmurHash2.
typedef cdbpp_base<murmurhash2> cdbpp;


};

/** @} */

/**
@mainpage C++ implementation of Constant Database (CDB++)

@section intro Introduction

Constant Database PlusPlus (CDB++) is a C++ implementation of hash database
specialized for serialization and retrieval of <i>static</i> associations
between keys and their values. The database provides several features:
- <b>Fast look-ups.</b> This library implements the data structure of the
  <a href="http://cr.yp.to/cdb.html">Constant Database</a> proposed by
  Daniel J. Bernstein.
- <b>Low footprint.</b> A CDB++ database consists of a chunk header (16 bytes),
  hash tables (2048 bytes and 16 bytes per record), and actual records (8 bytes
  plus key/value size per record).
- <b>Fast hash function.</b> CDB++ incorporates the fast and
  collision-resistant hash function for strings
  (<a href="http://murmurhash.googlepages.com/">MurmurHash 2.0</a>)
  implemented by Austin Appleby.
- <b>Chunk format.</b> The structure of CDB++ is designed to store the data in
  a chunk of a file; CDB++ database can be embedded into a file with other
  arbitrary data.
- <b>Simple write interface.</b> CDB++ can serialize a hash database to C++
  output streams (\c std::ostream).
- <b>Simple read interface.</b> CDB++ can prepare a hash database from an input
  stream (\c std::istream) or from a memory block on which a database image is
  read or memory-mapped from a file.
- <b>Cross platform.</b> The source code can be compiled on Microsoft Visual
  Studio 2008, GNU C Compiler (gcc), etc.
- <b>Very simple API.</b> The CDB++ API exposes only a few functions; one can
  use this library just by looking at the sample code.
- <b>Single C++ header implementation.</b> CDB++ is implemented in a single
  header file (cdbpp.h); one can use the CDB++ API only by including cdbpp.h
  in a source code.

CDB++ does not support these for simplicity:
- modifying associations
- checking collisions in keys
- compatibility of the database format on different byte-order architectures

@section sample Sample code
This sample code constructs a database "test.cdb" with 100,000 string/integer
associations, "000000"/0, "000001"/1, ..., "100000"/100000 (in build function).
Then the code issues string queries "000000", ..., "100000", and checks
whether the values are correct (in read function).

@include sample.cpp

@section download Download

- <a href="http://www.chokkan.org/software/dist/cdbpp-1.1.tar.gz">Source code</a>

CDB++ is distributed under the term of the
<a href="http://www.opensource.org/licenses/bsd-license.php">modified BSD license</a>.

@section changelog History
- Version 1.1 (2009-07-14):
    - Fixed a compile issue (a patch submitted by Takashi Imamichi).
    - Replaced SuperFastHash with MurmurHash 2.0 (a patch submitted by
      Takashi Imamichi).
    - Classes cdbpp::builder_base and cdbpp::cdbpp_base taking a template
      argument to configure a hash function. Classes cdbpp::builder and
      cdbpp::cdbpp are now the synonyms of
      \c cdbpp::builder_base<cdbpp::murmurhash2> and
      \c cdbpp::cdbpp_base<cdbpp::murmurhash2>, respectively.
    - Split the sample code into build and read functions.
- Version 1.0 (2009-07-09):
    - Initial release.

@section api Documentation

- @ref cdbpp_api "CDB++ API"

@section acknowledgements Acknowledgements

The data structure of the constant database was originally proposed by
<a href="http://cr.yp.to/djb.html">Daniel J. Bernstein</a>.

The source code of CDB++ includes the
<a href="http://murmurhash.googlepages.com/">MurmurHash 2.0</a>
implemented by Austin Appleby.

The CDB++ distribution contains "a portable stdint.h", which is released by
<a href="http://www.azillionmonkeys.com/qed/">Paul Hsieh</a> under the term of
the modified BSD license, for addressing the compatibility issue of Microsoft
Visual Studio 2008. The original code is available at:
http://www.azillionmonkeys.com/qed/pstdint.h

@section reference References
- <a href="http://cr.yp.to/cdb.html">cdb</a> by Daniel J. Bernstein.
- <a href="http://www.corpit.ru/mjt/tinycdb.html">TinyCDB - a Constant DataBase</a> by Michael Tokarev.
- <a href="http://cdbxx.sourceforge.net/">Constant Database C++ Bindings</a> by Stanislav Ievlev.
- <a href="http://www.unixuser.org/~euske/doc/cdbinternals/index.html">Constant Database (cdb) Internals</a> by Yusuke Shinyama.
- <a href="http://murmurhash.googlepages.com/">MurmurHash 2.0</a> by Austin Appleby.

*/

#endif/*__CDBPP_H__*/
