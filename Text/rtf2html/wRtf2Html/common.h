#ifndef __COMMON_H__
#define __COMMON_H__

#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

inline std::string from_int(int value)
{
   std::ostringstream buf;
   buf<<value;
   return buf.str();
}

inline std::string hex(unsigned int value)
{
   std::ostringstream buf;
   buf<<std::setw(2)<<std::setfill('0')<<std::hex<<value;
   return buf.str();
}

#endif
