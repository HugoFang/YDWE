#pragma warning(push, 3)
#include <lua.hpp>
#pragma warning(pop)
#include <base/i18n/gettext.h>	
#include <bee/utility/unicode.h>

namespace i18n {
	static std::wstring luaL_checkwstring(lua_State* L, int idx)
	{
		size_t len = 0;
		const char* buf = luaL_checklstring(L, idx, &len);
		return bee::u2w(std::string_view(buf, len));
	}

	static void luaL_pushwstring(lua_State* L, const std::wstring& str)
	{
		std::string utf8 = bee::w2u(str);
		lua_pushlstring(L, utf8.data(), utf8.size());
	}

	static int get_text(lua_State* L)
	{
		auto str = base::i18n::get_text(luaL_checkstring(L, 1));
		lua_pushlstring(L, str.data(), str.size());
		return 1;
	}

	static int set_domain(lua_State* L)
	{
		base::i18n::set_domain(luaL_checkwstring(L, 1));
		return 0;
	}

	static int get_language(lua_State* L)
	{
		luaL_pushwstring(L, base::i18n::get_language());
		return 1;
	}

	static int initialize(lua_State* L)
	{
		base::i18n::initialize(*(fs::path*)luaL_checkudata(L, 1, "bee::filesystem"));
		return 0;
	}
}

int luaopen_i18n(lua_State* L)
{
	luaL_Reg l[] = {
		{ "get_text", i18n::get_text },
		{ "set_domain", i18n::set_domain },
		{ "get_language", i18n::get_language },
		{ "initialize", i18n::initialize },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
