#pragma once

#include "yyjson.h"

template <ScriptContext context> void EncodeJSONTable(
	SQTable* table,
	yyjson_mut_doc* doc,
	yyjson_mut_val* root);

template <ScriptContext context> void
DecodeJsonTable(HSquirrelVM* sqvm, yyjson_val* obj);
