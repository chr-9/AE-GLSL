#include "GLSL.h"


typedef struct {
	unsigned long	index;
	char			str[256];
} TableString;


TableString		g_strs[StrID_NUMTYPES] = {
	StrID_NONE,						"",
	StrID_Name,						"AE-GLSL",
	StrID_Description,				"wip",
	StrID_Err_LoadSuite,			"Error loading suite.",
	StrID_Err_FreeSuite,			"Error releasing suite.",
};


char	*GetStringPtr(int strNum)
{
	return g_strs[strNum].str;
}

	