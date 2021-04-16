/*
 *      Memory-mapped-file implementation for Win32.
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

/* $Id: memory_mapped_file_win32.h 85 2010-02-23 17:23:34Z naoaki $ */

#ifndef __MEMORY_MAPPED_FILE_WIN32_H__
#define __MEMORY_MAPPED_FILE_WIN32_H__

#define NOMINMAX    // To fix min/max conflicts with STL.

#include <memory.h>
#include <windows.h>

class memory_mapped_file_win32 :
    public memory_mapped_file_base
{
public:
    typedef size_t size_type;

protected:
	HANDLE	                m_hFile;
	HANDLE	                m_hMapping;
    std::ios_base::openmode m_mode;
	char*	                m_data;
	size_type               m_size;

public:
    memory_mapped_file_win32()
    {
        m_hFile = INVALID_HANDLE_VALUE;
        m_hMapping = INVALID_HANDLE_VALUE;
        m_mode = 0;
        m_data = NULL;
        m_size = 0;
    }

    virtual ~memory_mapped_file_win32()
    {
        close();
    }

    void open(const std::string& path, std::ios_base::openmode mode)
    {
		DWORD dwDesiredAccess = 0;
		DWORD dwCreationDisposition = 0;

        if (mode & std::ios_base::in) {
			dwDesiredAccess |= GENERIC_READ;
			dwCreationDisposition = OPEN_EXISTING;
		}
        if (mode & std::ios_base::out) {
			dwDesiredAccess |= GENERIC_WRITE;
			dwCreationDisposition = CREATE_NEW;
		}
		if (mode & std::ios_base::trunc) {
			dwDesiredAccess = (GENERIC_READ | GENERIC_WRITE);
			dwCreationDisposition = CREATE_ALWAYS;
		}

		m_hFile = CreateFileA(
			path.c_str(),
			dwDesiredAccess,
			0,
			NULL,
			dwCreationDisposition,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);

		if (m_hFile != INVALID_HANDLE_VALUE) {
            m_mode = mode;
            this->resize((size_type)GetFileSize(m_hFile, NULL));
        }
	}

    bool is_open() const
    {
        return (m_hFile != INVALID_HANDLE_VALUE);
    }

    void close()
    {
        this->free();
        if (m_hFile != INVALID_HANDLE_VALUE) {
		    CloseHandle(m_hFile);
		    m_hFile = INVALID_HANDLE_VALUE;
        }
	}

    bool resize(size_type size)
    {
	    if (size == 0) {
            this->free();
		    return true;
	    }

        if (m_hFile == INVALID_HANDLE_VALUE) {
            return false;
        }

        this->free();
        DWORD flProtect = (m_mode & std::ios_base::out) ? PAGE_READWRITE : PAGE_READONLY;
        m_hMapping = CreateFileMappingA(
		    m_hFile,
		    NULL,
            flProtect,
		    0,
		    (DWORD)size,
		    NULL
		    );

	    if (m_hMapping == NULL) {
		    CloseHandle(m_hFile);
		    m_hFile = NULL;
            return false;
	    }

        DWORD dwDesiredAccess = (m_mode & std::ios_base::out) ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
        m_data = (char*)MapViewOfFile(
		    m_hMapping,
		    dwDesiredAccess,
		    0,
		    0,
		    0
		    );

	    if (m_data == NULL) {
		    CloseHandle(m_hMapping);
		    m_hMapping = NULL;
		    CloseHandle(m_hFile);
		    m_hFile = NULL;
            return false;
	    }

	    m_size = size;
        return true;
    }

    void free()
    {
	    if (m_data != NULL) {
		    UnmapViewOfFile(m_data);
		    m_data = NULL;
	    }
	    if (m_hMapping != INVALID_HANDLE_VALUE) {
		    CloseHandle(m_hMapping);
		    m_hMapping = NULL;
	    }
	    m_size = 0;
    }

    size_type size() const
    {
        return m_size;
    }

    char* data() const
    {
        return m_data;
    }

    const char* const_data() const
    {
        return m_data;
    }
   
    static int alignment()
    {
        return 0;
    }
};

#endif/*__MEMORY_MAPPED_FILE_WIN32_H__*/