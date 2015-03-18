
//Example for cowyard.inc

#define FILTERSCRIPT

#include <a_samp>
#include <streamer>
#include <foreach>

#define CREATE_YARD_BORDERS

#include "cowyard"

public OnFilterScriptInit()
{
    CreateCowYard(10,
				  -1209.133544, -1216.067260,
				  -1188.992553, -1194.817260,
				   128.7, 0, 0);

	return 1;
}

