/*
 *      Memory-mapped-file library compatible with Win32 and POSIX.
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

/* $Id: memory_mapped_file.h 85 2010-02-23 17:23:34Z naoaki $ */

#ifndef __MEMORY_MAPPED_FILE_H__
#define __MEMORY_MAPPED_FILE_H__

#include <iostream>
#include <string>

class memory_mapped_file_base
{
public:
    typedef size_t size_type;

    memory_mapped_file_base() {}
    virtual ~memory_mapped_file_base() {}

    void open(const std::string& path, std::ios_base::openmode mode) {}
    bool is_open() const {return false; }
    void close() {}
    void resize(size_type size) {}
    size_type size() const {return 0; }
    char* data() const {return NULL; }
    const char* const_data() const {return NULL; }
    static int alignment() {return 0; }
};

#if     defined(_WIN32)
#include "memory_mapped_file_win32.h"
#define memory_mapped_file memory_mapped_file_win32

#else
#include "memory_mapped_file_posix.h"
#define memory_mapped_file memory_mapped_file_posix

#endif

#endif/*__MEMORY_MAPPED_FILE_H__*/
