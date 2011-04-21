#include "script.h"

#include <dlib/log.h>

#include "script_private.h"
#include "script_hash.h"
#include "script_msg.h"
#include "script_vmath.h"
#include "script_sys.h"

extern "C"
{
#include <lua/lauxlib.h>
#include <lua/lualib.h>
}

namespace dmScript
{
    HContext NewContext()
    {
        Context* context = new Context();
        context->m_Descriptors.SetCapacity(17, 128);
        return context;
    }

    void DeleteContext(HContext context)
    {
        delete context;
    }

    ScriptParams::ScriptParams()
    : m_Context(0x0)
    , m_SetURLsCallback(0x0)
    {

    }

    void Initialize(lua_State* L, const ScriptParams& params)
    {
        InitializeHash(L);
        InitializeMsg(L);
        InitializeVmath(L);
        InitializeSys(L);

        lua_pushlightuserdata(L, (void*)params.m_SetURLsCallback);
        lua_setglobal(L, SCRIPT_GET_URLS_CALLBACK);

        lua_pushlightuserdata(L, (void*)params.m_Context);
        lua_setglobal(L, SCRIPT_CONTEXT);
    }

    bool RegisterDDFType(HContext context, const dmDDF::Descriptor* descriptor)
    {
        if (context->m_Descriptors.Full())
        {
            dmLogError("Unable to register ddf type. Out of resources.");
            return false;
        }
        else
        {
            context->m_Descriptors.Put(dmHashString64(descriptor->m_Name), descriptor);
            return true;
        }
    }
}
