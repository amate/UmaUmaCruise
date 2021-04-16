/*
 *      SimString.
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

/* $Id: simstring.h 106 2010-03-07 08:15:49Z naoaki $ */

#ifndef __SIMSTRING_H__
#define __SIMSTRING_H__

#include <stdint.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ngram.h"
#include "measure.h"
#include "cdbpp.h"
#include "memory_mapped_file.h"

#define	SIMSTRING_NAME           "SimString"
#define	SIMSTRING_COPYRIGHT      "Copyright (c) 2009,2010 Naoaki Okazaki"
#define	SIMSTRING_MAJOR_VERSION  1
#define SIMSTRING_MINOR_VERSION  0
#define SIMSTRING_STREAM_VERSION 2

/** 
 * \addtogroup api SimString C++ API
 * @{
 *
 *  The SimString C++ API.
 */

namespace simstring
{

enum {
    BYTEORDER_CHECK = 0x62445371,
};

/**
 * Query types.
 */
enum {
    /// Exact match.
    exact = 0,
    /// Approximate string matching with dice coefficient.
    dice,
    /// Approximate string matching with cosine coefficient.
    cosine,
    /// Approximate string matching with jaccard coefficient.
    jaccard,
    /// Approximate string matching with overlap coefficient.
    overlap,
};



/**
 * A writer for an n-gram database.
 *  This template class builds an n-gram database. The first template
 *  argument (string_tmpl) specifies the type of a key (string), the second
 *  template argument (value_tmpl) specifies the type of a value associated
 *  with a key, and the third template argument (ngram_generator_tmpl)
 *  customizes generation of feature sets (n-grams) from keys.
 *
 *  This class is inherited by writer_base, which adds the functionality of
 *  managing a master string table (list of strings).
 *
 *  @param  string_tmpl             The type of a string.
 *  @param  value_tmpl              The value type.
 *                                  This is required to be an integer type.
 *  @param  ngram_generator_tmpl    The type of an n-gram generator.
 */
template <
    class string_tmpl,
    class value_tmpl,
    class ngram_generator_tmpl
>
class ngramdb_writer_base
{
public:
    /// The type representing a string.
    typedef string_tmpl string_type;
    /// The type of values associated with key strings.
    typedef value_tmpl value_type;
    /// The function type for generating n-grams from a key string.
    typedef ngram_generator_tmpl ngram_generator_type;
    /// The type representing a character.
    typedef typename string_type::value_type char_type;

protected:
    /// The type of an array of n-grams.
    typedef std::vector<string_type> ngrams_type;
    /// The vector type of values associated with an n-gram.
    typedef std::vector<value_type> values_type;
    /// The type implementing an index (associations from n-grams to values).
    typedef std::map<string_type, values_type> hashdb_type;
    /// The vector of indices for different n-gram sizes.
    typedef std::vector<hashdb_type> indices_type;

protected:
    /// The vector of indices.
    indices_type m_indices;
    /// The n-gram generator.
    const ngram_generator_type& m_gen;
    /// The error message.
    std::stringstream m_error;

public:
    /**
     * Constructs an object.
     *  @param  gen             The n-gram generator.
     */
    ngramdb_writer_base(const ngram_generator_type& gen)
        : m_gen(gen)
    {
    }

    /**
     * Destructs an object.
     */
    virtual ~ngramdb_writer_base()
    {
    }

    /**
     * Clears the database.
     */
    void clear()
    {
        m_indices.clear();
        m_error.str("");
    }

    /**
     * Checks whether the database is empty.
     *  @return bool    \c true if the database is empty, \c false otherwise.
     */
    bool empty()
    {
        return m_indices.empty();
    }

    /**
     * Returns the maximum length of keys in the n-gram database.
     *  @return int     The maximum length of keys.
     */
    int max_size() const
    {
        return (int)m_indices.size();
    }

    /**
     * Checks whether an error has occurred.
     *  @return bool    \c true if an error has occurred.
     */
    bool fail() const
    {
        return !m_error.str().empty();
    }

    /**
     * Returns an error message.
     *  @return std::string The string of the error message.
     */
    std::string error() const
    {
        return m_error.str();
    }

    /**
     * Inserts a string to the n-gram database.
     *  @param  key         The key string.
     *  @param  value       The value associated with the string.
     */
    bool insert(const string_type& key, const value_type& value)
    {
        // Generate n-grams from the key string.
        ngrams_type ngrams;
        m_gen(key, std::back_inserter(ngrams));
        if (ngrams.empty()) {
            return false;
        }

        // Resize the index array for the number of the n-grams;
        // we build an index for each n-gram number.
        if (m_indices.size() < ngrams.size()) {
            m_indices.resize(ngrams.size());
        }
        hashdb_type& index = m_indices[ngrams.size()-1];

        // Store the associations from the n-grams to the value.
        typename ngrams_type::const_iterator it;
        for (it = ngrams.begin();it != ngrams.end();++it) {
            const string_type& ngram = *it;
            typename hashdb_type::iterator iti = index.find(ngram);
            if (iti == index.end()) {
                // Create a new posting array.
                values_type v(1);
                v[0] = value;
                index.insert(typename hashdb_type::value_type(ngram, v));
            } else {
                // Append the value to the existing posting array.
                iti->second.push_back(value);
            }
        }

        return true;
    }

    /**
     * Stores the n-gram database to files.
     *  @param  name        The prefix of file names.
     *  @return bool        \c true if the database is successfully stored,
     *                      \c false otherwise.
     */
    bool store(const std::string& base)
    {
        // Write out all the indices to files.
        for (int i = 0;i < (int)m_indices.size();++i) {
            if (!m_indices[i].empty()) {
                std::stringstream ss;
                ss << base << '.' << i+1 << ".cdb";
                bool b = this->store(ss.str(), m_indices[i]);
                if (!b) {
                    return false;
                }
            }
        }

        return true;
    }

protected:
    bool store(const std::string& name, const hashdb_type& index)
    {
        // Open the database file with binary mode.
        std::ofstream ofs(name.c_str(), std::ios::binary);
        if (ofs.fail()) {
            m_error << "Failed to open a file for writing: " << name;
            return false;
        }

        try {
            // Open a CDB++ writer.
            cdbpp::builder dbw(ofs);

            // Put associations: n-gram -> values.
            typename hashdb_type::const_iterator it;
            for (it = index.begin();it != index.end();++it) {
                // Put an association from an n-gram to its values. 
                dbw.put(
                    it->first.c_str(),
                    sizeof(char_type) * it->first.length(),
                    &it->second[0],
                    sizeof(it->second[0]) * it->second.size()
                    );
            }

        } catch (const cdbpp::builder_exception& e) {
            m_error << "CDB++ error: " << e.what();
            return false;
        }

        return true;
    }
};



/**
 * A SimString database writer.
 *  This template class builds a SimString database. The first template
 *  argument (string_tmpl) specifies the type of a character, and the second
 *  template argument (ngram_generator_tmpl) customizes generation of feature
 *  sets (n-grams) from strings.
 *
 *  Inheriting the base class ngramdb_writer_base that builds indices from
 *  n-grams to string IDs, this class maintains associations between strings
 *  and string IDs.
 *
 *  @param  string_tmpl             The type of a string.
 *  @param  ngram_generator_tmpl    The type of an n-gram generator.
 */
template <
    class string_tmpl,
    class ngram_generator_tmpl = ngram_generator
>
class writer_base :
    public ngramdb_writer_base<string_tmpl, uint32_t, ngram_generator_tmpl>
{
public:
    /// The type representing a string.
    typedef string_tmpl string_type;
    /// The type of values associated with key strings.
    typedef uint32_t value_type;
    /// The function type for generating n-grams from a key string.
    typedef ngram_generator_tmpl ngram_generator_type;
    /// The type representing a character.
    typedef typename string_type::value_type char_type;
    // The type of the base class.
    typedef ngramdb_writer_base<string_tmpl, uint32_t, ngram_generator_tmpl> base_type;

protected:
    /// The base name of the database.
    std::string m_name;
    /// The output stream for the string collection.
    std::ofstream m_ofs;
    /// The number of strings in the database.
    int m_num_entries;

public:
    /**
     * Constructs a writer object.
     *  @param  gen         The n-gram generator used by this writer.
     */
    writer_base(const ngram_generator_type& gen)
        : base_type(gen), m_num_entries(0)
    {
    }

    /**
     * Constructs a writer object by opening a database.
     *  @param  gen         The n-gram generator used by this writer.
     *  @param  name        The name of the database.
     */
    writer_base(
        const ngram_generator_type& gen,
        const std::string& name
        )
        : base_type(gen), m_num_entries(0)
    {
        this->open(name);
    }

    /**
     * Destructs a writer object.
     */
    virtual ~writer_base()
    {
        close();
    }

    /**
     * Opens a database.
     *  @param  name        The name of the database.
     *  @return bool        \c true if the database is successfully opened,
     *                      \c false otherwise.
     */
    bool open(const std::string& name)
    {
        m_num_entries = 0;

        // Open the master file for writing.
        m_ofs.open(name.c_str(), std::ios::binary);
        if (m_ofs.fail()) {
            this->m_error << "Failed to open a file for writing: " << name;
            return false;
        }

        // Reserve the region for a file header.
        if (!this->write_header(m_ofs)) {
            m_ofs.close();
            return false;
        }

        m_name = name;
        return true;
    }

    /**
     * Closes the database.
     *  @param  name        The name of the database.
     *  @return bool        \c true if the database is successfully opened,
     *                      \c false otherwise.
     */
    bool close()
    {
        bool b = true;

        // Write the n-gram database to files.
        if (!m_name.empty()) {
            b &= this->store(m_name);
        }

        // Finalize the file header, and close the file.
        if (m_ofs.is_open()) {
            b &= this->write_header(m_ofs);
            m_ofs.close();
        }

        // Initialize the members.
        m_name.clear();
        m_num_entries = 0;
        return b;
    }

    /**
     * Inserts a string to the database.
     *  @param  str         The string to be inserted.
     *  @return bool        \c true if the string is successfully inserted,
     *                      \c false otherwise.
     */
    bool insert(const string_type& str)
    {
        // This will be the offset address to access the key string.
        value_type off = (value_type)(std::streamoff)m_ofs.tellp();

        // Write the key string to the master file.
        m_ofs.write(reinterpret_cast<const char*>(str.c_str()), sizeof(char_type) * (str.length()+1));
        if (m_ofs.fail()) {
            this->m_error << "Failed to write a string to the master file.";
            return false;
        }
        ++m_num_entries;

        // Insert the n-grams of the key string to the database.
        return base_type::insert(str, off);
    }

protected:
    bool write_header(std::ofstream& ofs)
    {
        uint32_t num_entries = m_num_entries;
        uint32_t max_size = (uint32_t)this->max_size();
        uint32_t size = (uint32_t)m_ofs.tellp();

        // Seek to the beginning of the master file, to which the file header
        // is to be written.
        ofs.seekp(0);
        if (ofs.fail()) {
            this->m_error << "Failed to seek the file pointer for the master file.";
            return false;
        }

        // Write the file header.
        m_ofs.write("SSDB", 4);
        write_uint32(BYTEORDER_CHECK);
        write_uint32(SIMSTRING_STREAM_VERSION);
        write_uint32(size);
        write_uint32(sizeof(char_type));
        write_uint32(this->m_gen.get_n());
        write_uint32(static_cast<int>(this->m_gen.get_be()));
        write_uint32(num_entries);
        write_uint32(max_size);
        if (ofs.fail()) {
            this->m_error << "Failed to write a file header to the master file.";
            return false;
        }

        return true;
    }

    inline void write_uint32(uint32_t value)
    {
        m_ofs.write(reinterpret_cast<const char *>(&value), sizeof(value));
    }
};



/**
 * A reader for an n-gram database.
 *  @param  value_tmpl              The value type.
 *                                  This is required to be an integer type.
 */
template <
    class value_tmpl
>
class ngramdb_reader_base
{
public:
    /// The type of a value.
    typedef value_tmpl value_type;
    
protected:
    // An inverted list of SIDs.
    struct inverted_list_type
    {
        int num;
        const value_type* values;

        friend bool operator<(
            const inverted_list_type& x, 
            const inverted_list_type& y
            )
        {
            return (x.num < y.num);
        }
    };
    // An array of inverted lists.
    typedef std::vector<inverted_list_type> inverted_lists_type;

    // A hash table that retrieves SIDs from n-grams.
    typedef cdbpp::cdbpp hashtbl_type;

    // An index containing strings of the same size.
    struct index_type
    {
        // The memory image of the database.
        memory_mapped_file  image;
        // The index.
        hashtbl_type        table;
    };

    // Indices with different sizes of strings.
    typedef std::vector<index_type> indices_type;

    // A candidate string of retrieved results.
    struct candidate_type
    {
        // The SID.
        value_type  value;
        // The overlap count (frequency of the SID in the inverted lists).
        int         num;

        candidate_type(value_type v, int n)
            : value(v), num(n)
        {
        }
    };

    // An array of candidates.
    typedef std::vector<candidate_type> candidates_type;

    // An array of SIDs retrieved.
    typedef std::vector<value_type> results_type;

protected:
    // The array of the indices.
    indices_type m_indices;
    // The maximum size of strings in the database.
    int m_max_size;
    // The database name (base name of indices).
    std::string m_name;
    // The error message.
    std::stringstream m_error;


public:
    /**
     * Constructs an object.
     */
    ngramdb_reader_base()
    {
    }

    /**
     * Destructs an object.
     */
    virtual ~ngramdb_reader_base()
    {
    }

    /**
     * Checks whether an error has occurred.
     *  @return bool    \c true if an error has occurred.
     */
    bool fail() const
    {
        return !m_error.str().empty();
    }

    /**
     * Returns an error message.
     *  @return std::string The string of the error message.
     */
    std::string error() const
    {
        return m_error.str();
    }

    /**
     * Opens an n-gram database.
     *  @param  name        The name of the database.
     *  @param  max_size    The maximum size of the strings.
     */
    void open(const std::string& name, int max_size)
    {
        m_name = name;
        m_max_size = max_size;
        // The maximum size corresponds to the number of indices in the database.
        m_indices.resize(max_size);
    }

    /**
     * Closes an n-gram database.
     */
    void close()
    {
        m_name.clear();
        m_indices.clear();
        m_error.str("");
    }

    /**
     * Performs an overlap join on inverted lists retrieved for the query.
     *  @param  query       The query object that stores query n-grams,
     *                      threshold, and conditions for the similarity
     *                      measure.
     *  @param  results     The SIDs that satisfies the overlap join.
     */
    template <class measure_type, class query_type>
    void overlapjoin(const query_type& query, double alpha, results_type& results)
    {
        int i;
        const int qsize = query.size();

        // Allocate a vector of postings corresponding to n-gram queries.
        inverted_lists_type posts(qsize);

        // Compute the range of n-gram lengths for the candidate strings;
        // in other words, we do not have to search for strings whose n-gram
        // lengths are out of this range.
        const int xmin = std::max(measure_type::min_size(query.size(), alpha), 1);
        const int xmax = std::min(measure_type::max_size(query.size(), alpha), m_max_size);

        // Loop for each length in the range.
        for (int xsize = xmin;xsize <= xmax;++xsize) {
            // Access to the n-gram index for the length.
            hashtbl_type& tbl = open_index(m_name, xsize);
            if (!tbl.is_open()) {
                // Ignore an empty index.
                continue;
            }

            // Search for string entries that match to each query n-gram.
            // Note that we do not traverse each entry here, but only obtain
            // the number of and the pointer to the entries.
            typename query_type::const_iterator it;
            for (it = query.begin(), i = 0;it != query.end();++it, ++i) {
                size_t vsize;
                const void *values = tbl.get(
                    it->c_str(),
                    sizeof(it->at(0)) * it->length(),
                    &vsize
                    );
                posts[i].num = (int)(vsize / sizeof(value_type));
                posts[i].values = reinterpret_cast<const value_type*>(values);
            }

            // Sort the query n-grams by ascending order of their frequencies.
            // This reduces the number of initial candidates.
            std::sort(posts.begin(), posts.end());

            // The minimum number of n-gram matches required for the query.
            const int mmin = measure_type::min_match(qsize, xsize, alpha);
            // A candidate must match to one of n-grams in these queries.
            const int min_queries = qsize - mmin + 1;

            // Step 1: collect candidates that match to the initial queries.
            candidates_type cands;
            for (i = 0;i < min_queries;++i) {
                candidates_type tmp;
                typename candidates_type::const_iterator itc = cands.begin();
                const value_type* p = posts[i].values;
                const value_type* last = posts[i].values + posts[i].num;

                while (itc != cands.end() || p != last) {
                    if (itc == cands.end() || (p != last && itc->value > *p)) {
                        tmp.push_back(candidate_type(*p, 1));
                        ++p;
                    } else if (p == last || (itc != cands.end() && itc->value < *p)) {
                        tmp.push_back(candidate_type(itc->value, itc->num));
                        ++itc;
                    } else {
                        tmp.push_back(candidate_type(itc->value, itc->num+1));
                        ++itc;
                        ++p;
                    }
                }
                std::swap(cands, tmp);
            }

            // No initial candidate is found.
            if (cands.empty()) {
                continue;
            }

            // Step 2: count the number of matches with remaining queries.
            for (;i < qsize;++i) {
                candidates_type tmp;
                typename candidates_type::const_iterator itc;
                const value_type* first = posts[i].values;
                const value_type* last = posts[i].values + posts[i].num;

                // For each active candidate.
                for (itc = cands.begin();itc != cands.end();++itc) {
                    int num = itc->num;
                    if (std::binary_search(first, last, itc->value)) {
                        ++num;
                    }

                    if (mmin <= num) {
                        // This candidate has sufficient matches.
                        results.push_back(itc->value);
                    } else if (num + (qsize - i - 1) >= mmin) {
                        // This candidate still has the chance.
                        tmp.push_back(candidate_type(itc->value, num));
                    }
                }
                std::swap(cands, tmp);

                // Exit the loop if all candidates are pruned.
                if (cands.empty()) {
                    break;
                }
            }

            if (!cands.empty()) {
                // Step 2 was not performed.
                typename candidates_type::const_iterator itc;
                for (itc = cands.begin();itc != cands.end();++itc) {
                    if (mmin <= itc->num) {
                        results.push_back(itc->value);
                    }
                }
            }
        }
    }

protected:
    /**
     * Open the index storing strings of the specific size.
     *  @param  base            The base name of the indices.
     *  @param  size            The size of strings.
     *  @return hashtbl_type&   The hash table of the index.
     */
    hashtbl_type& open_index(const std::string& base, int size)
    {
        index_type& index = m_indices[size-1];
        if (!index.table.is_open()) {
            std::stringstream ss;
            ss << base << '.' << size << ".cdb";
            index.image.open(ss.str().c_str(), std::ios::in);
            if (index.image.is_open()) {
                index.table.open(index.image.data(), index.image.size());
            }
        }

        return index.table;
    }
};



/**
 * A SimString database reader.
 *  This template class retrieves string from a SimString database.
 *
 *  Inheriting the base class ngramdb_reader_base that retrieves string IDs
 *  from a query feature set, this class manages the master string table,
 *  which maintains associations between strings and string IDs.
 */
class reader
    : public ngramdb_reader_base<uint32_t>
{
public:
    /// The type of an n-gram generator.
    typedef ngram_generator ngram_generator_type;
    /// The type of the base class.
    typedef ngramdb_reader_base<uint32_t> base_type;

protected:
    int m_ngram_unit;
    bool m_be;
    int m_char_size;

    /// The content of the master file.
    std::vector<char> m_strings;

public:
    /**
     * Constructs an object.
     */
    reader()
    {
    }

    /**
     * Destructs an object.
     */
    virtual ~reader()
    {
        close();
    }

    /**
     * Opens a SimString database.
     *  @param  name        The name of the SimString database.
     *  @return bool        \c true if the database is successfully opened,
     *                      \c false otherwise.
     */
    bool open(const std::string& name)
    {
        uint32_t num_entries, max_size;

        // Open the master file.
        std::ifstream ifs(name.c_str(), std::ios_base::in | std::ios_base::binary);
        if (ifs.fail()) {
            this->m_error << "Failed to open the master file: " << name;
            return false;
        }

        // Obtain the size of the master file.
        ifs.seekg(0, std::ios_base::end);
        size_t size = (size_t)ifs.tellg();
        ifs.seekg(0, std::ios_base::beg);
        
        // Read the image of the master file.
        m_strings.resize(size);
        ifs.read(&m_strings[0], size);
        ifs.close();

        // Check the file header.
        const char* p = &m_strings[0];
        if (size < 36 || std::strncmp(p, "SSDB", 4) != 0) {
            this->m_error << "Incorrect file format";
            return false;
        }
        p += 4;

        // Check the byte order.
        if (BYTEORDER_CHECK != read_uint32(p)) {
            this->m_error << "Incompatible byte order";
            return false;
        }
        p += 4;

        // Check the version.
        if (SIMSTRING_STREAM_VERSION != read_uint32(p)) {
            this->m_error << "Incompatible stream version";
            return false;
        }
        p += 4;

        // Check the chunk size.
        if (size != read_uint32(p)) {
            this->m_error << "Inconsistent chunk size";
            return false;
        }
        p += 4;

        // Read the unit of n-grams, begin/end flag.
        m_char_size = (int)read_uint32(p);
        p += 4;
        m_ngram_unit = (int)read_uint32(p);
        p += 4;
        m_be = (read_uint32(p) != 0);
        p += 4;

        // Read the number of enties.
        num_entries = read_uint32(p);
        p += 4;

        // Read the maximum size of strings in the database.
        max_size = read_uint32(p);

        base_type::open(name, (int)max_size);
        return true;
    }

    /**
     * Closes the database.
     */
    void close()
    {
        base_type::close();
    }

    int char_size() const
    {
        return m_char_size;
    }

    /**
     * Retrieves strings that are similar to the query.
     *  @param  query           The query string.
     *  @param  measure         The similarity measure.
     *  @param  alpha           The threshold for approximate string matching.
     *  @param  ins             The insert iterator that receives retrieved
     *                          strings.
     *  @see    ::simstring::exact, ::simstring::dice, ::simstring::cosine,
     *          ::simstring::jaccard, ::simstring::overlap
     */
    template <class string_type, class insert_iterator>
    void retrieve(
        const string_type& query,
        int measure,
        double alpha,
        insert_iterator ins
        )
    {
        switch (measure) {
        case exact:
            this->retrieve<simstring::measure::exact>(query, alpha, ins);
            break;
        case dice:
            this->retrieve<simstring::measure::dice>(query, alpha, ins);
            break;
        case cosine:
            this->retrieve<simstring::measure::cosine>(query, alpha, ins);
            break;
        case jaccard:
            this->retrieve<simstring::measure::jaccard>(query, alpha, ins);
            break;
        case overlap:
            this->retrieve<simstring::measure::overlap>(query, alpha, ins);
            break;
        }
    }

    /**
     * Retrieves strings that are similar to the query.
     *  @param  measure_type    The similarity measure.
     *  @param  query           The query string.
     *  @param  alpha           The threshold for approximate string matching.
     *  @param  ins             The insert iterator that receives retrieved
     *                          strings.
     *  @see    ::simstring::measure::exact, ::simstring::measure::dice,
     *          ::simstring::measure::cosine, ::simstring::measure::jaccard,
     *          ::simstring::measure::overlap
     */
    template <class measure_type, class string_type, class insert_iterator>
    void retrieve(
        const string_type& query,
        double alpha,
        insert_iterator ins
        )
    {
        typedef std::vector<string_type> ngrams_type;
        typedef typename string_type::value_type char_type;

        ngram_generator_type gen(m_ngram_unit, m_be);
        ngrams_type ngrams;
        gen(query, std::back_inserter(ngrams));

        typename base_type::results_type results;
        base_type::overlapjoin<measure_type>(ngrams, alpha, results);

        typename base_type::results_type::const_iterator it;
        const char* strings = &m_strings[0];
        for (it = results.begin();it != results.end();++it) {
            const char_type* xstr = reinterpret_cast<const char_type*>(strings + *it);
            *ins = xstr;
        }
    }


protected:
    inline uint32_t read_uint32(const char* p) const
    {
        return *reinterpret_cast<const uint32_t*>(p);
    }
};

};

/** @} */

/**
@mainpage SimString - A fast and efficient implementation for approximate string matching

@section documentation Documentation

- @ref api "SimString C++ API"

@section sample Sample Programs

A basic sample.

@include sample.cpp

A Unicode sample.

@include sample_unicode.cpp

*/

#endif/*__SIMSTRING_H__*/
