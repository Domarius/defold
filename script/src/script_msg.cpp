#include "script.h"

#include "script_private.h"

#include <stdint.h>
#include <string.h>

#include <dlib/dstrings.h>
#include <dlib/message.h>

#include <ddf/ddf.h>

extern "C"
{
#include <lua/lauxlib.h>
#include <lua/lualib.h>
}

namespace dmScript
{
#define SCRIPT_LIB_NAME "msg"
#define SCRIPT_TYPE_NAME_URL "url"

    const uint32_t MAX_MESSAGE_DATA_SIZE = 256;

    bool IsURL(lua_State *L, int index)
    {
        void *p = lua_touserdata(L, index);
        bool result = false;
        if (p != 0x0)
        {  /* value is a userdata? */
            if (lua_getmetatable(L, index))
            {  /* does it have a metatable? */
                lua_getfield(L, LUA_REGISTRYINDEX, SCRIPT_TYPE_NAME_URL);  /* get correct metatable */
                if (lua_rawequal(L, -1, -2))
                {  /* does it have the correct mt? */
                    result = true;
                }
                lua_pop(L, 2);  /* remove both metatables */
            }
        }
        return result;
    }

    static int URL_gc(lua_State *L)
    {
        dmMessage::URL* url = CheckURL(L, 1);
        memset(url, 0, sizeof(*url));
        return 0;
    }

    void url_tostring(const dmMessage::URL* url, char* buffer, uint32_t buffer_size)
    {
        *buffer = '\0';
        if (url->m_Socket != 0)
        {
            const char* socket = dmMessage::GetSocketName(url->m_Socket);
            if (socket != 0x0)
            {
                dmStrlCpy(buffer, socket, buffer_size);
                dmStrlCat(buffer, ":", buffer_size);
            }
            else
            {
                dmStrlCpy(buffer, "unknown:", buffer_size);
            }
        }
        if (url->m_Path != 0)
        {
            char path[16];
            DM_SNPRINTF(path, 16, "%llu", url->m_Path);
            dmStrlCat(buffer, path, buffer_size);
        }
        if (url->m_Fragment != 0)
        {
            dmStrlCat(buffer, "#", buffer_size);
            char path[16];
            DM_SNPRINTF(path, 16, "%llu", url->m_Fragment);
            dmStrlCat(buffer, path, buffer_size);
        }
    }

    static int URL_tostring(lua_State *L)
    {
        dmMessage::URL* url = CheckURL(L, 1);
        char buffer[64];
        url_tostring(url, buffer, 64);
        lua_pushfstring(L, "%s: [%s]", SCRIPT_TYPE_NAME_URL, buffer);
        return 1;
    }

    static int URL_concat(lua_State *L)
    {
        const char* s = luaL_checkstring(L, 1);
        dmMessage::URL* url = CheckURL(L, 2);
        char buffer[64];
        url_tostring(url, buffer, 64);
        lua_pushfstring(L, "%s[%s]", s, buffer);
        return 1;
    }

    static int URL_index(lua_State *L)
    {
        dmMessage::URL* url = CheckURL(L, 1);

        const char* key = luaL_checkstring(L, 2);
        if (strcmp("socket", key) == 0)
        {
            if (url->m_Socket != 0)
            {
                lua_pushnumber(L, url->m_Socket);
            }
            else
            {
                lua_pushnil(L);
            }
            return 1;
        }
        else if (strcmp("path", key) == 0)
        {
            if (url->m_Path != 0)
            {
                PushHash(L, url->m_Path);
            }
            else
            {
                lua_pushnil(L);
            }
            return 1;
        }
        else if (strcmp("fragment", key) == 0)
        {
            if (url->m_Fragment != 0)
            {
                PushHash(L, url->m_Fragment);
            }
            else
            {
                lua_pushnil(L);
            }
            return 1;
        }
        else
        {
            return luaL_error(L, "%s.%s only has fields socket, path, fragment.", SCRIPT_LIB_NAME, SCRIPT_TYPE_NAME_URL);
        }
    }

    static int URL_newindex(lua_State *L)
    {
        dmMessage::URL* url = CheckURL(L, 1);

        const char* key = luaL_checkstring(L, 2);
        if (strcmp("socket", key) == 0)
        {
            if (lua_isnumber(L, 3))
            {
                url->m_Socket = (dmMessage::HSocket)luaL_checknumber(L, 3);
                if (dmMessage::GetSocketName(url->m_Socket) == 0x0)
                {
                    return luaL_error(L, "Could not find the socket in %d.", url->m_Socket);
                }
            }
            else if (lua_isstring(L, 3))
            {
                const char* socket_name = lua_tostring(L, 3);
                dmMessage::Result result = dmMessage::GetSocket(socket_name, &url->m_Socket);
                if (result != dmMessage::RESULT_OK)
                {
                    return luaL_error(L, "Could not find the socket '%s'.", socket_name);
                }
            }
            else if (lua_isnil(L, 3))
            {
                url->m_Socket = 0;
            }
            else
            {
                return luaL_error(L, "Invalid type for socket, must be number, string or nil.");
            }
        }
        else if (strcmp("path", key) == 0)
        {
            if (lua_isstring(L, 3))
            {
                url->m_Path = dmHashString64(lua_tostring(L, 3));
            }
            else if (lua_isnil(L, 3))
            {
                url->m_Path = 0;
            }
            else if (IsHash(L, 3))
            {
                url->m_Path = CheckHash(L, 3);
            }
            else
            {
                return luaL_error(L, "Invalid type for path, must be hash, string or nil.");
            }
        }
        else if (strcmp("fragment", key) == 0)
        {
            if (lua_isstring(L, 3))
            {
                url->m_Fragment = dmHashString64(lua_tostring(L, 3));
            }
            else if (lua_isnil(L, 3))
            {
                url->m_Fragment = 0;
            }
            else if (IsHash(L, 3))
            {
                url->m_Fragment = CheckHash(L, 3);
            }
            else
            {
                return luaL_error(L, "Invalid type for fragment, must be hash, string or nil.");
            }
        }
        else
        {
            return luaL_error(L, "%s.%s only has fields socket, path, fragment.", SCRIPT_LIB_NAME, SCRIPT_TYPE_NAME_URL);
        }
        return 0;
    }

    static int URL_eq(lua_State *L)
    {
        dmMessage::URL* url1 = CheckURL(L, 1);
        dmMessage::URL* url2 = CheckURL(L, 2);
        lua_pushboolean(L, url1->m_Socket == url2->m_Socket && url1->m_Path == url2->m_Path && url1->m_Fragment == url2->m_Fragment);
        return 1;
    }

    static const luaL_reg URL_methods[] =
    {
        {0,0}
    };

    static const luaL_reg URL_meta[] =
    {
        {"__gc",        URL_gc},
        {"__tostring",  URL_tostring},
        {"__concat",    URL_concat},
        {"__index",     URL_index},
        {"__newindex",  URL_newindex},
        {"__eq",        URL_eq},
        {0,0}
    };

    int URL_new(lua_State* L)
    {
        int top = lua_gettop(L);
        dmMessage::URL url;
        if (top == 1 && !lua_isnil(L, 1))
        {
            const char* s = luaL_checkstring(L, 1);
            dmMessage::Result result = dmMessage::ParseURL(s, &url);
            switch (result)
            {
                case dmMessage::RESULT_OK:
                    break;
                case dmMessage::RESULT_MALFORMED_URL:
                    return luaL_error(L, "Error when parsing '%s', must be of the format 'socket:path#fragment'.", s);
                case dmMessage::RESULT_INVALID_SOCKET_NAME:
                    return luaL_error(L, "The socket name in '%s' is invalid.", s);
                case dmMessage::RESULT_SOCKET_NOT_FOUND:
                    return luaL_error(L, "The socket in '%s' could not be found.", s);
                default:
                    return luaL_error(L, "Error when parsing '%s': %d.", s, result);
            }
        }
        else if (top == 3)
        {
            if (!lua_isnil(L, 1))
            {
                if (lua_isnumber(L, 1))
                {
                    url.m_Socket = lua_tonumber(L, 1);
                }
                else
                {
                    const char* s = lua_tostring(L, 1);
                    dmMessage::Result result = dmMessage::GetSocket(s, &url.m_Socket);
                    switch (result)
                    {
                        case dmMessage::RESULT_OK:
                            break;
                        case dmMessage::RESULT_INVALID_SOCKET_NAME:
                            return luaL_error(L, "The socket '%s' is invalid.", s);
                        case dmMessage::RESULT_SOCKET_NOT_FOUND:
                            return luaL_error(L, "The socket '%s' could not be found.", s);
                        default:
                            return luaL_error(L, "Error when checking socket '%s': %d.", s, result);
                    }
                }
            }
            if (!lua_isnil(L, 2))
            {
                if (lua_isstring(L, 2))
                {
                    url.m_Path = dmHashString64(lua_tostring(L, 2));
                }
                else
                {
                    url.m_Path = CheckHash(L, 2);
                }
            }
            if (!lua_isnil(L, 3))
            {
                if (lua_isstring(L, 3))
                {
                    url.m_Fragment = dmHashString64(lua_tostring(L, 3));
                }
                else
                {
                    url.m_Fragment = CheckHash(L, 3);
                }
            }
        }
        else if (top > 0 && !lua_isnil(L, 1))
        {
            luaL_error(L, "Only %s.%s(\"[socket:][path][#fragment]\") or %s.%s(socket, path, fragment) is supported.", SCRIPT_LIB_NAME, SCRIPT_TYPE_NAME_URL, SCRIPT_LIB_NAME, SCRIPT_TYPE_NAME_URL);
        }
        PushURL(L, url);
        assert(top + 1 == lua_gettop(L));
        return 1;
    }

    int Msg_Post(lua_State* L)
    {
        int top = lua_gettop(L);

        dmMessage::URL sender;
        dmMessage::URL receiver;

        if (lua_isstring(L, 2))
        {
            const char* url = lua_tostring(L, 2);
            dmMessage::Result result = dmMessage::ParseURL(url, &receiver);
            if (result != dmMessage::RESULT_OK)
            {
                const char* error = "Could not send message to %s because %s.";
                switch (result)
                {
                    case dmMessage::RESULT_SOCKET_NOT_FOUND:
                        return luaL_error(L, error, url, "the socket could not be found");
                    case dmMessage::RESULT_INVALID_SOCKET_NAME:
                        return luaL_error(L, error, url, "the socket name is invalid");
                    case dmMessage::RESULT_MALFORMED_URL:
                        return luaL_error(L, error, url, "the address is invalid (should be [socket:][path][#fragment])");
                    default:
                        break;
                }
            }
        }
        else
        {
            receiver = *CheckURL(L, 2);
        }
        lua_getglobal(L, SCRIPT_GET_URLS_CALLBACK);
        SetURLsCallback callback = (SetURLsCallback)lua_touserdata(L, -1);
        lua_pop(L, 1);
        assert(callback != 0x0);
        if (!callback(L, 1, &sender, &receiver))
        {
            return luaL_error(L, "The self reference is invalid.");
        }
        dmhash_t message_id;
        if (lua_isstring(L, 3))
        {
            message_id = dmHashString64(lua_tostring(L, 3));
        }
        else
        {
            message_id = CheckHash(L, 3);
        }

        uintptr_t descriptor = 0;

        char data[MAX_MESSAGE_DATA_SIZE];
        uint32_t data_size = 0;

        if (top > 3)
        {
            lua_getglobal(L, SCRIPT_CONTEXT);
            HContext context = (HContext)lua_touserdata(L, -1);
            lua_pop(L, 1);
            const dmDDF::Descriptor** desc = context->m_Descriptors.Get(message_id);
            if (desc != 0)
            {
                descriptor = (uintptr_t)*desc;
                if ((*desc)->m_Size > MAX_MESSAGE_DATA_SIZE)
                {
                    return luaL_error(L, "The message is too large to be sent (%d bytes, max is %d).", (*desc)->m_Size, MAX_MESSAGE_DATA_SIZE);
                }
                luaL_checktype(L, 4, LUA_TTABLE);

                lua_pushvalue(L, 4);
                data_size = dmScript::CheckDDF(L, *desc, data, MAX_MESSAGE_DATA_SIZE, 4);
                lua_pop(L, 1);
            }
            else
            {
                data_size = dmScript::CheckTable(L, data, MAX_MESSAGE_DATA_SIZE, 4);
            }
        }

        assert(top == lua_gettop(L));

        dmMessage::Result result = dmMessage::Post(&sender, &receiver, message_id, descriptor, data, data_size);
        if (result != dmMessage::RESULT_OK)
        {
            return luaL_error(L, "Could not send message to %s.", dmMessage::GetSocketName(receiver.m_Socket));
        }

        return 0;
    }

    static const luaL_reg ScriptMsg_methods[] =
    {
        {SCRIPT_TYPE_NAME_URL, URL_new},
        {"post", Msg_Post},
        {0, 0}
    };

    void InitializeMsg(lua_State* L)
    {
        int top = lua_gettop(L);

        const uint32_t type_count = 1;
        struct
        {
            const char* m_Name;
            const luaL_reg* m_Methods;
            const luaL_reg* m_Metatable;
        } types[type_count] =
        {
            {SCRIPT_TYPE_NAME_URL, URL_methods, URL_meta}
        };
        for (uint32_t i = 0; i < type_count; ++i)
        {
            // create methods table, add it to the globals
            luaL_register(L, types[i].m_Name, types[i].m_Methods);
            int methods_index = lua_gettop(L);
            // create metatable for the type, add it to the Lua registry
            luaL_newmetatable(L, types[i].m_Name);
            int metatable = lua_gettop(L);
            // fill metatable
            luaL_register(L, 0, types[i].m_Metatable);

            lua_pushliteral(L, "__metatable");
            lua_pushvalue(L, methods_index);// dup methods table
            lua_settable(L, metatable);

            lua_pop(L, 2);
        }
        luaL_register(L, SCRIPT_LIB_NAME, ScriptMsg_methods);
        lua_pop(L, 1);

        assert(top == lua_gettop(L));
    }

    void PushURL(lua_State* L, const dmMessage::URL& url)
    {
        dmMessage::URL* urlp = (dmMessage::URL*)lua_newuserdata(L, sizeof(dmMessage::URL));
        *urlp = url;
        luaL_getmetatable(L, SCRIPT_TYPE_NAME_URL);
        lua_setmetatable(L, -2);
    }

    dmMessage::URL* CheckURL(lua_State* L, int index)
    {
        if (lua_type(L, index) == LUA_TUSERDATA)
        {
            return (dmMessage::URL*)luaL_checkudata(L, index, SCRIPT_TYPE_NAME_URL);
        }
        luaL_typerror(L, index, SCRIPT_TYPE_NAME_URL);
        return 0x0;
    }

#undef SCRIPT_LIB_NAME
#undef SCRIPT_TYPE_NAME_URL
}
