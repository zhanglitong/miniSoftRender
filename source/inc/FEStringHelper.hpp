#pragma     once

#include    <stdio.h>
#include    <string.h>
#include    <stdlib.h>
#include	"FEDefine.h"

namespace   FE
{
	class	FEStringHelper
	{
	public:
		static	Strings	split(const String& str, const char delim = ',')
		{
			if (str.empty())
				return	{};
			Strings result;
			size_t	si	=	0;
			size_t	ei	=	str.find(delim, si);
			if (ei == String::npos) 
			{
				return {str};
			}
			while (ei != String::npos) 
			{
				result.push_back(str.substr(si, ei - si));
				si = ei + 1;
				ei = str.find(delim, si);
			}

			if (si < str.length()) 
			{
				result.push_back(str.substr(si));
			}
			return result;
		}
	};
	
}
