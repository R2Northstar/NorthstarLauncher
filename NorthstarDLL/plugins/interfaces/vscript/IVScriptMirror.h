#ifndef IVSCRIPT_H
#define IVSCRIPT_H

#include "squirrel/squirrel.h"

#define VSCRIPT_CLIENT_VERSION "NSVScriptClient001"
#define VSCRIPT_SERVER_VERSION "NSVScripServer001"

// TODO not virtual, maybe a template?
class IVScriptMirror
{
  public:
	sq_getboolType sq_getbool = 0;
	sq_pushboolType sq_pushbool = 0;
};

#endif
