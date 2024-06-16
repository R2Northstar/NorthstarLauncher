#include "core/json.h"
#include "squirrel/squirrel.h"

#include "yyjson.h"

#ifdef _MSC_VER
#undef GetObject // fuck microsoft developers
#endif

// TODO
template <ScriptContext context> void
DecodeJsonArray(HSquirrelVM* sqvm, yyjson_val* arr)
{
	g_pSquirrel<context>->newarray(sqvm, 0);

	size_t idx, max;
	yyjson_val* val;
	yyjson_arr_foreach(arr, idx, max, val)
	{
		switch (yyjson_get_type(val))
		{
		case YYJSON_TYPE_OBJ:
			DecodeJsonTable<context>(sqvm, val);
			g_pSquirrel<context>->arrayappend(sqvm, -2);
			break;
		case YYJSON_TYPE_ARR:
			DecodeJsonArray<context>(sqvm, val);
			g_pSquirrel<context>->arrayappend(sqvm, -2);
			break;
		case YYJSON_TYPE_STR:
			g_pSquirrel<context>->pushstring(sqvm, yyjson_get_str(val), -1);
			g_pSquirrel<context>->arrayappend(sqvm, -2);
			break;
		case YYJSON_TYPE_BOOL:
			g_pSquirrel<context>->pushbool(sqvm, yyjson_get_bool(val));
			g_pSquirrel<context>->arrayappend(sqvm, -2);
			break;
		case YYJSON_TYPE_NUM:
			switch (yyjson_get_subtype(val))
			{
				case YYJSON_SUBTYPE_UINT:
				case YYJSON_SUBTYPE_SINT:
					g_pSquirrel<context>->pushinteger(sqvm, yyjson_get_int(val));
					break;

				case YYJSON_SUBTYPE_REAL:
					g_pSquirrel<context>->pushfloat(sqvm, yyjson_get_real(val));
					break;

				default:
					break;
			}

			g_pSquirrel<context>->arrayappend(sqvm, -2);
			break;
		case YYJSON_TYPE_NULL:
			break;
		}
	}
}

template <ScriptContext context> void
DecodeJsonTable(HSquirrelVM* sqvm, yyjson_val* obj)
{
	g_pSquirrel<context>->newtable(sqvm);

	size_t idx, max;
	yyjson_val* key;
	yyjson_val* val;
	yyjson_obj_foreach(obj, idx, max, key, val)
	{
		const char* key_name = yyjson_get_str(key);

		switch (yyjson_get_type(val))
		{
		case YYJSON_TYPE_OBJ:
			g_pSquirrel<context>->pushstring(sqvm, key_name, -1);
			DecodeJsonTable<context>(sqvm, val);
			g_pSquirrel<context>->newslot(sqvm, -3, false);
			break;
		case YYJSON_TYPE_ARR:
			g_pSquirrel<context>->pushstring(sqvm, key_name, -1);
			DecodeJsonArray<context>(sqvm, val);
			g_pSquirrel<context>->newslot(sqvm, -3, false);
			break;
		case YYJSON_TYPE_STR:
			g_pSquirrel<context>->pushstring(sqvm, key_name, -1);
			g_pSquirrel<context>->pushstring(sqvm, yyjson_get_str(val), -1);

			g_pSquirrel<context>->newslot(sqvm, -3, false);
			break;
		case YYJSON_TYPE_BOOL:
			g_pSquirrel<context>->pushstring(sqvm, key_name, -1);
			g_pSquirrel<context>->pushbool(sqvm, yyjson_get_bool(val));
			g_pSquirrel<context>->newslot(sqvm, -3, false);
			break;
		case YYJSON_TYPE_NUM:
			switch (yyjson_get_subtype(val))
			{
				case YYJSON_SUBTYPE_UINT:
				case YYJSON_SUBTYPE_SINT:
					g_pSquirrel<context>->pushstring(sqvm, key_name, -1);
					g_pSquirrel<context>->pushinteger(sqvm, yyjson_get_int(val));
					break;

				case YYJSON_SUBTYPE_REAL:
					g_pSquirrel<context>->pushstring(sqvm, key_name, -1);
					g_pSquirrel<context>->pushfloat(sqvm, yyjson_get_real(val));
					break;

				default:
					break;
			}

			g_pSquirrel<context>->newslot(sqvm, -3, false);
			break;
		case YYJSON_TYPE_NULL:
			break;
		}
	}
}

template <ScriptContext context> void EncodeJSONTable(
	SQTable* table,
	yyjson_mut_doc* doc,
	yyjson_mut_val* root)
{
	for (int i = 0; i < table->_numOfNodes; i++)
	{
		tableNode* node = &table->_nodes[i];
		if (node->key._Type == OT_STRING)
		{
			switch (node->val._Type)
			{
			case OT_STRING:
				yyjson_mut_obj_add_str(doc, root, node->key._VAL.asString->_val, node->val._VAL.asString->_val);
				break;
			case OT_INTEGER:
				yyjson_mut_obj_add_int(doc, root, node->key._VAL.asString->_val, node->val._VAL.asInteger);
				break;
			case OT_FLOAT:
				yyjson_mut_obj_add_real(doc, root, node->key._VAL.asString->_val, node->val._VAL.asFloat);
				break;
			case OT_BOOL:
				if (node->val._VAL.asInteger)
					yyjson_mut_obj_add_true(doc, root, node->key._VAL.asString->_val);
				else
					yyjson_mut_obj_add_false(doc, root, node->key._VAL.asString->_val);
				break;
			case OT_TABLE:
				{
					yyjson_mut_val* new_obj = yyjson_mut_obj_add_obj(doc, root, node->key._VAL.asString->_val);
					EncodeJSONTable<context>(node->val._VAL.asTable, doc, new_obj);
				}
				break;
			case OT_ARRAY:
				{
					yyjson_mut_val* new_arr = yyjson_mut_obj_add_arr(doc, root, node->key._VAL.asString->_val);
					EncodeJSONArray<context>(node->val._VAL.asArray, doc, new_arr);
				}
				break;
			default:
				spdlog::warn("SQ_EncodeJSON: squirrel type {} not supported", SQTypeNameFromID(node->val._Type));
				break;
			}
		}
	}
}

template <ScriptContext context> void EncodeJSONArray(
	SQArray* arr,
	yyjson_mut_doc* doc,
	yyjson_mut_val* root)
{
	for (int i = 0; i < arr->_usedSlots; i++)
	{
		SQObject* node = &arr->_values[i];

		switch (node->_Type)
		{
		case OT_STRING:
			yyjson_mut_arr_add_str(doc, root, node->_VAL.asString->_val);
			break;
		case OT_INTEGER:
			yyjson_mut_arr_add_sint(doc, root, node->_VAL.asInteger);
			break;
		case OT_FLOAT:
			yyjson_mut_arr_add_real(doc, root, node->_VAL.asFloat);
			break;
		case OT_BOOL:
			if (node->_VAL.asInteger)
				yyjson_mut_arr_add_true(doc, root);
			else
				yyjson_mut_arr_add_false(doc, root);
			break;
		case OT_TABLE:
			{
				yyjson_mut_val* new_obj = yyjson_mut_arr_add_obj(doc, root);
				EncodeJSONTable<context>(node->_VAL.asTable, doc, new_obj);
			}
			break;
		case OT_ARRAY:
			{
				yyjson_mut_val* new_arr = yyjson_mut_arr_add_arr(doc, root);
				EncodeJSONArray<context>(node->_VAL.asArray, doc, new_arr);
			}
			break;
		default:
			spdlog::info("SQ encode Json type {} not supported", SQTypeNameFromID(node->_Type));
		}
	}
}

ADD_SQFUNC(
	"table",
	DecodeJSON,
	"string json, bool fatalParseErrors = false",
	"converts a json string to a squirrel table",
	ScriptContext::UI | ScriptContext::CLIENT | ScriptContext::SERVER)
{
	const char* pJson = g_pSquirrel<context>->getstring(sqvm, 1);
	const bool bFatalParseErrors = g_pSquirrel<context>->getbool(sqvm, 2);

	yyjson_read_err err;
	yyjson_doc* doc = yyjson_read_opts(const_cast<char*>(pJson), strlen(pJson), 9, &YYJSON_ALLOCATOR, &err);
	if (!doc)
	{
		g_pSquirrel<context>->newtable(sqvm);

		std::string sErrorString = fmt::format(
			"Failed parsing json string: encountered parse error \"{}\" at offset {}",
			err.msg,
			err.pos);

		if (bFatalParseErrors)
		{
			g_pSquirrel<context>->raiseerror(sqvm, sErrorString.c_str());
			return SQRESULT_ERROR;
		}

		spdlog::warn(sErrorString);
		return SQRESULT_NOTNULL;
	}

	yyjson_val* root = yyjson_doc_get_root(doc);

	if (!yyjson_is_obj(root))
	{
		const char* sErrorString = "given json string is not an object";

		if (bFatalParseErrors)
		{
			g_pSquirrel<context>->raiseerror(sqvm, sErrorString);
			return SQRESULT_ERROR;
		}

		spdlog::warn(sErrorString);
		return SQRESULT_NOTNULL;
	}

	DecodeJsonTable<context>(sqvm, root);

	yyjson_doc_free(doc);

	return SQRESULT_NOTNULL;
}

ADD_SQFUNC(
	"string",
	EncodeJSON,
	"table data",
	"converts a squirrel table to a json string",
	ScriptContext::UI | ScriptContext::CLIENT | ScriptContext::SERVER)
{
	yyjson_mut_doc* doc = yyjson_mut_doc_new(&YYJSON_ALLOCATOR);
	yyjson_mut_val* root = yyjson_mut_obj(doc);
	yyjson_mut_doc_set_root(doc, root);

	// temp until this is just the func parameter type
	HSquirrelVM* vm = (HSquirrelVM*)sqvm;
	SQTable* table = vm->_stackOfCurrentFunction[1]._VAL.asTable;
	EncodeJSONTable<context>(table, doc, root);

	const char* pJsonString = yyjson_mut_write(doc, 0, NULL);

	g_pSquirrel<context>->pushstring(sqvm, pJsonString, -1);

	_free_base((void*)pJsonString);
	yyjson_mut_doc_free(doc);

	return SQRESULT_NOTNULL;
}
