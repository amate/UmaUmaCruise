/*
 *      Memory-mapped-file implementation for POSIX.
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

/* $Id: memory_mapped_file_posix.h 85 2010-02-23 17:23:34Z naoaki $ */

#ifndef __MEMORY_MAPPED_FILE_POSIX_H__
#define __MEMORY_MAPPED_FILE_POSIX_H__

#include <fcntl.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

class memory_mapped_file_posix :
    public memory_mapped_file_base
{
public:
    typedef size_t size_type;

protected:
    int                     m_fd;
    std::ios_base::openmode m_mode;
    void*                   m_data;
    size_type               m_size;

public:
    memory_mapped_file_posix()
    {
        m_fd = -1;
        m_mode = std::ios_base::in;
        m_data = NULL;
        m_size = 0;
    }

    virtual ~memory_mapped_file_posix()
    {
        close();
    }

    void open(const std::string& path, std::ios_base::openmode mode)
    {
        int flags = 0;
        struct stat buf;

        if (mode & std::ios_base::in) {
            flags = O_RDONLY;
        }
        if (mode & std::ios_base::out) {
            flags = O_RDWR | O_CREAT;
        }
        if (mode & std::ios_base::trunc) {
            flags |= (O_RDWR | O_TRUNC);
        }

        m_fd = ::open(path.c_str(), flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (m_fd != -1) {
            if (::fstat(m_fd, &buf) == 0) {
                m_mode = mode;
                this->resize((size_type)buf.st_size);
            } else {
                ::close(m_fd);
                m_fd = -1;
            }
        }
    }

    bool is_open() const
    {
        return (m_fd != -1);
    }

    void close()
    {
        this->free();
        if (m_fd != -1) {
            ::close(m_fd);
            m_fd = -1;
        }
    }

    bool resize(size_type size)
    {
        if (size == 0) {
            this->free();
            return true;
        }

        if (m_fd == -1) {
            return false;
        }

        this->free();

        if ((m_mode & std::ios_base::out) && m_size < size) {
            /* Try to expand the file to the specified size. */
            if (::lseek(m_size, size, SEEK_SET) >= 0) {
                char c;
                if (read(m_fd, &c, sizeof(char)) == -1) {
                    c = 0;
                }
                if (write(m_fd, &c, sizeof(char)) == -1) {
                    return false;   // Failed to write the last position.
                }
            } else {
                return false;       // Failed to expand the file.
            }
        }

        /* Map the file into process memory. */
        m_data = ::mmap(
            NULL,
            size,
            (m_mode & std::ios_base::out) ? (PROT_READ | PROT_WRITE) : PROT_READ,
            MAP_SHARED,
            m_fd,
            0);

        m_size = size;
        return true;
    }

    void free()
    {
        if (m_data != NULL) {
            ::munmap(m_data, m_size);   
            m_data = NULL;
        }
        m_size = 0;
    }

    size_type size() const
    {
        return m_size;
    }

    char* data() const
    {
        return reinterpret_cast<char*>(m_data);
    }

    const char* const_data() const
    {
        return reinterpret_cast<const char*>(m_data);
    }
   
    static int alignment()
    {
        return 0;
    }
};

#endif/*__MEMORY_MAPPED_FILE_POSIX_H__*/
