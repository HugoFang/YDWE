#include <lua.hpp>	  
#include <base/win/registry/v2/key.h> 
#include <base/util/unicode.h>


#define RKEY_TRY() try   
#define RKEY_TRY_END() catch (const std::exception& e) { lua_pushstring(L, e.what()); return lua_error(L); }

namespace luawarp { namespace registry {

	using namespace base::registry::v2;

	key_w* rkey_read(lua_State* L, int idx)
	{
		return static_cast<key_w*>(lua_touserdata(L, idx));
	}

	std::wstring rkey_read_wstring(lua_State* L, int idx)
	{
		return base::util::u2w(std::string_view(lua_tostring(L, idx), lua_rawlen(L, idx)), base::util::conv_method::replace | '?');
	}
	
	int rkey_push_wstring(lua_State* L, const std::wstring& value)
	{
		std::string utf8_value = base::util::w2u(value, base::util::conv_method::replace | '?');
		lua_pushlstring(L, utf8_value.data(), utf8_value.size());
		return 1;
	}

	int rkey_push_blob(lua_State* L, const std::dynarray<uint8_t>& value)
	{
		lua_pushlstring(L, (const char*)value.data(), value.size());
		return 1;
	}

	int rkey_create(lua_State* L, int mt_idx, key_w::hkey_type keytype)
	{
		void* storage = lua_newuserdata(L, sizeof(key_w));
		lua_pushvalue(L, mt_idx);
		lua_setmetatable(L, -2);
		new (storage)key_w(keytype);
		return 1;
	}

	int rkey_copy(lua_State* L, int ud_idx, const key_w* key)
	{
		void* storage = lua_newuserdata(L, sizeof(key_w));
		lua_getmetatable(L, ud_idx);
		lua_setmetatable(L, -2);
		new (storage)key_w(*key);
		return 1;
	}

	int rkey_index(lua_State* L)
	{
		RKEY_TRY()
		{
			key_w*       self = rkey_read(L, 1);
			std::wstring key = rkey_read_wstring(L, 2);
			key_w::value_type& value = self->value(key);
			switch (value.type())
			{
			case REG_DWORD:
				lua_pushinteger(L, value.get_uint32_t());
				return 1;
			case REG_QWORD:
				lua_pushinteger(L, value.get_uint64_t());
				return 1;
			case REG_SZ:
			case REG_EXPAND_SZ:
				return rkey_push_wstring(L, value.get_string());
			case REG_BINARY:
				return rkey_push_blob(L, value.get_binary());
			default:
				break;
			}
			lua_pushnil(L);
			return 1;
		}
		RKEY_TRY_END();
	}

	int rkey_newindex(lua_State* L)
	{
		RKEY_TRY()
		{
			key_w*       self = rkey_read(L, 1);
			std::wstring key = rkey_read_wstring(L, 2);
			key_w::value_type& value = self->value(key);
			lua_geti(L, 3, 1);
			lua_geti(L, 3, 2);
			switch (lua_tointeger(L, -2))
			{
			case REG_DWORD:
				value.set_uint32_t((uint32_t)lua_tointeger(L, -1));
				break;
			case REG_QWORD:
				value.set_uint64_t(lua_tointeger(L, -1));
				break;
			case REG_SZ:
			case REG_EXPAND_SZ:
				value.set(rkey_read_wstring(L, -1));
				break;
			case REG_BINARY:
			{
							   size_t buflen = 0;
							   const char* buf = lua_tolstring(L, -1, &buflen);
							   value.set((const void*)buf, buflen);
							   break;
			}
			default:
				break;
			}
			return 0;
		}
		RKEY_TRY_END();
	}

	int rkey_div(lua_State* L)
	{
		RKEY_TRY()
		{
			key_w*       self = rkey_read(L, 1);
			std::wstring rht = rkey_read_wstring(L, 2);
			key_w        ret = *self / rht;
			return rkey_copy(L, 1, &ret);
		}
		RKEY_TRY_END();
	}

	int rkey_gc(lua_State* L)
	{
		static_cast<key_w*>(lua_touserdata(L, 1))->~key_w();
		return 0;
	}

	int current_user(lua_State* L)
	{
		return rkey_create(L, lua_upvalueindex(1), HKEY_CURRENT_USER);
	}

	int local_machine(lua_State* L)
	{
		return rkey_create(L, lua_upvalueindex(1), HKEY_LOCAL_MACHINE);
	}
}}

int luaopen_registry(lua_State* L)
{
	using namespace luawarp;
	static luaL_Reg func[] = {
		{ "current_user", registry::current_user },
		{ "local_machine", registry::local_machine },
		{ NULL, NULL }
	};
	static luaL_Reg rkey_mt[] = {
		{ "__index", registry::rkey_index },
		{ "__newindex", registry::rkey_newindex },
		{ "__div", registry::rkey_div },
		{ "__gc", registry::rkey_gc },
		{ NULL, NULL }
	};
	luaL_newlibtable(L, func);
	luaL_newlib(L, rkey_mt);
	luaL_setfuncs(L, func, 1);

#define LUA_PUSH_CONST(L, val) \
	lua_pushinteger(L, (val)); \
	lua_setfield(L, -2, # val);

	LUA_PUSH_CONST(L, REG_DWORD);
	LUA_PUSH_CONST(L, REG_QWORD);
	LUA_PUSH_CONST(L, REG_SZ);
	LUA_PUSH_CONST(L, REG_EXPAND_SZ);
	LUA_PUSH_CONST(L, REG_BINARY);

#undef LUA_PUSH_CONST

	lua_setglobal(L, "registry");
	return 0;
}