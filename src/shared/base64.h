/////////////////////////////////////////////////
// base64.h

#pragma once

#include <string>
#include <atlalloc.h>

namespace my_Base64
{

std::string base64_encode(const unsigned char* bytes_to_encode, unsigned long len);
unsigned long base64_decode(const std::string& s, ATL::CTempBuffer<unsigned char>* ret = NULL, unsigned long nBuf = 0);

}
