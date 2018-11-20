// Code by Matt Stevens
// https://github.com/mattstevens/dmap-parser
//
//
// Copyright (c) 2011-2013 Matt Stevens
// 
// 
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#pragma once

#include "stdint_win.h"

class CDmapParser
{
public:
	CDmapParser(void);
	~CDmapParser(void);

	static const char *dmap_name_from_code(const char *code);
	int dmap_parse(void* ctx, const char *buf, size_t len);

protected:
	virtual void on_dict_start	(void* ctx, const char *code, const char *name) {}
	virtual void on_dict_end	(void* ctx, const char *code, const char *name) {}
	virtual void on_int32		(void* ctx, const char *code, const char *name, int32_t value) {}
	virtual void on_int64		(void* ctx, const char *code, const char *name, int64_t value) {}
	virtual void on_uint32		(void* ctx, const char *code, const char *name, uint32_t value) {}
	virtual void on_uint64		(void* ctx, const char *code, const char *name, uint64_t value) {}
	virtual void on_date		(void* ctx, const char *code, const char *name, uint32_t value) {}
	virtual void on_string		(void* ctx, const char *code, const char *name, const char *buf, size_t len)  {}
	virtual void on_data		(void* ctx, const char *code, const char *name, const char *buf, size_t len)  {}
};

