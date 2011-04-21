#include <setjmp.h>
#include <stdlib.h>
#include <dlib/log.h>
#include <gtest/gtest.h>
#include "../script.h"
#include "test/test_ddf.h"

extern "C"
{
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>
}

class LuaTableTest* g_LuaTableTest = 0;

class LuaTableTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        accept_panic = false;
        g_LuaTableTest = this;
        L = lua_open();
        lua_atpanic(L, &AtPanic);
        dmScript::Initialize(L, dmScript::ScriptParams());
        top = lua_gettop(L);
    }

    static int AtPanic(lua_State *L)
    {
        if (g_LuaTableTest->accept_panic)
            longjmp(g_LuaTableTest->env, 0);
        dmLogError("Unexpected error: %s", lua_tostring(L, -1));
        exit(5);
        return 0;
    }

    virtual void TearDown()
    {
        ASSERT_EQ(top, lua_gettop(L));
        lua_close(L);
        g_LuaTableTest = 0;
    }

    bool accept_panic;
    jmp_buf env;
    int top;
    lua_State* L;
};

TEST_F(LuaTableTest, EmptyTable)
{
    lua_newtable(L);
    char buf[1];
    uint32_t buffer_used = dmScript::CheckTable(L, buf, sizeof(buf), -1);
    ASSERT_EQ(1U, buffer_used);
    lua_pop(L, 1);
}

TEST_F(LuaTableTest, Table01)
{
    // Create table
    lua_newtable(L);
    lua_pushinteger(L, 123);
    lua_setfield(L, -2, "a");

    lua_pushinteger(L, 456);
    lua_setfield(L, -2, "b");

    char buf[256];

    uint32_t buffer_used = dmScript::CheckTable(L, buf, sizeof(buf), -1);
    (void) buffer_used;
    lua_pop(L, 1);

    dmScript::PushTable(L, buf);

    lua_getfield(L, -1, "a");
    ASSERT_EQ(LUA_TNUMBER, lua_type(L, -1));
    ASSERT_EQ(123, lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "b");
    ASSERT_EQ(LUA_TNUMBER, lua_type(L, -1));
    ASSERT_EQ(456, lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_pop(L, 1);

    // Create table again
    lua_newtable(L);
    lua_pushinteger(L, 123);
    lua_setfield(L, -2, "a");
    lua_pushinteger(L, 456);
    lua_setfield(L, -2, "b");

    int ret = setjmp(env);
    if (ret == 0)
    {
        // buffer_user - 1, expect error
        accept_panic = true;
        dmScript::CheckTable(L, buf, buffer_used-1, -1);
        ASSERT_TRUE(0); // Never reached due to error
    }
    else
    {
        lua_pop(L, 1);
    }
}

TEST_F(LuaTableTest, Table02)
{
    // Create table
    lua_newtable(L);
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, "foo");

    lua_pushstring(L, "kalle");
    lua_setfield(L, -2, "foo2");

    char buf[256];

    uint32_t buffer_used = dmScript::CheckTable(L, buf, sizeof(buf), -1);
    (void) buffer_used;
    lua_pop(L, 1);

    dmScript::PushTable(L, buf);

    lua_getfield(L, -1, "foo");
    ASSERT_EQ(LUA_TBOOLEAN, lua_type(L, -1));
    ASSERT_EQ(1, lua_toboolean(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "foo2");
    ASSERT_EQ(LUA_TSTRING, lua_type(L, -1));
    ASSERT_STREQ("kalle", lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_pop(L, 1);

    // Create table again
    lua_newtable(L);
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, "foo");

    lua_pushstring(L, "kalle");
    lua_setfield(L, -2, "foo2");

    int ret = setjmp(env);
    if (ret == 0)
    {
        // buffer_user - 1, expect error
        accept_panic = true;
        dmScript::CheckTable(L, buf, buffer_used-1, -1);
        ASSERT_TRUE(0); // Never reached due to error
    }
    else
    {
        lua_pop(L, 1);
    }
}

TEST_F(LuaTableTest, Vector3)
{
    // Create table
    lua_newtable(L);
    dmScript::PushVector3(L, Vectormath::Aos::Vector3(1,2,3));
    lua_setfield(L, -2, "v");

    char buf[256];

    uint32_t buffer_used = dmScript::CheckTable(L, buf, sizeof(buf), -1);
    (void) buffer_used;
    lua_pop(L, 1);

    dmScript::PushTable(L, buf);

    lua_getfield(L, -1, "v");
    ASSERT_TRUE(dmScript::IsVector3(L, -1));
    Vectormath::Aos::Vector3* v = dmScript::CheckVector3(L, -1);
    ASSERT_EQ(1, v->getX());
    ASSERT_EQ(2, v->getY());
    ASSERT_EQ(3, v->getZ());
    lua_pop(L, 1);

    lua_pop(L, 1);
}

TEST_F(LuaTableTest, Vector4)
{
    // Create table
    lua_newtable(L);
    dmScript::PushVector4(L, Vectormath::Aos::Vector4(1,2,3,4));
    lua_setfield(L, -2, "v");

    char buf[256];

    uint32_t buffer_used = dmScript::CheckTable(L, buf, sizeof(buf), -1);
    (void) buffer_used;
    lua_pop(L, 1);

    dmScript::PushTable(L, buf);

    lua_getfield(L, -1, "v");
    ASSERT_TRUE(dmScript::IsVector4(L, -1));
    Vectormath::Aos::Vector4* v = dmScript::CheckVector4(L, -1);
    ASSERT_EQ(1, v->getX());
    ASSERT_EQ(2, v->getY());
    ASSERT_EQ(3, v->getZ());
    ASSERT_EQ(4, v->getW());
    lua_pop(L, 1);

    lua_pop(L, 1);
}

TEST_F(LuaTableTest, Quat)
{
    // Create table
    lua_newtable(L);
    dmScript::PushQuat(L, Vectormath::Aos::Quat(1,2,3,4));
    lua_setfield(L, -2, "v");

    char buf[256];

    uint32_t buffer_used = dmScript::CheckTable(L, buf, sizeof(buf), -1);
    (void) buffer_used;
    lua_pop(L, 1);

    dmScript::PushTable(L, buf);

    lua_getfield(L, -1, "v");
    ASSERT_TRUE(dmScript::IsQuat(L, -1));
    Vectormath::Aos::Quat* v = dmScript::CheckQuat(L, -1);
    ASSERT_EQ(1, v->getX());
    ASSERT_EQ(2, v->getY());
    ASSERT_EQ(3, v->getZ());
    ASSERT_EQ(4, v->getW());
    lua_pop(L, 1);

    lua_pop(L, 1);
}

TEST_F(LuaTableTest, Matrix4)
{
    // Create table
    lua_newtable(L);
    Vectormath::Aos::Matrix4 m;
    for (uint32_t i = 0; i < 4; ++i)
        for (uint32_t j = 0; j < 4; ++j)
            m.setElem(i, j, i * 4 + j);
    dmScript::PushMatrix4(L, m);
    lua_setfield(L, -2, "v");

    char buf[256];

    uint32_t buffer_used = dmScript::CheckTable(L, buf, sizeof(buf), -1);
    (void) buffer_used;
    lua_pop(L, 1);

    dmScript::PushTable(L, buf);

    lua_getfield(L, -1, "v");
    ASSERT_TRUE(dmScript::IsMatrix4(L, -1));
    Vectormath::Aos::Matrix4* v = dmScript::CheckMatrix4(L, -1);
    for (uint32_t i = 0; i < 4; ++i)
        for (uint32_t j = 0; j < 4; ++j)
            ASSERT_EQ(i * 4 + j, v->getElem(i, j));
    lua_pop(L, 1);

    lua_pop(L, 1);
}

TEST_F(LuaTableTest, MixedKeys)
{
    // Create table
    lua_newtable(L);

    lua_pushnumber(L, 1);
    lua_pushnumber(L, 2);
    lua_settable(L, -3);

    lua_pushstring(L, "key1");
    lua_pushnumber(L, 3);
    lua_settable(L, -3);

    lua_pushnumber(L, 2);
    lua_pushnumber(L, 4);
    lua_settable(L, -3);

    lua_pushstring(L, "key2");
    lua_pushnumber(L, 5);
    lua_settable(L, -3);

    char buf[256];

    uint32_t buffer_used = dmScript::CheckTable(L, buf, sizeof(buf), -1);
    (void) buffer_used;
    lua_pop(L, 1);

    dmScript::PushTable(L, buf);

    lua_pushnumber(L, 1);
    lua_gettable(L, -2);
    ASSERT_EQ(LUA_TNUMBER, lua_type(L, -1));
    ASSERT_EQ(2, lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_pushstring(L, "key1");
    lua_gettable(L, -2);
    ASSERT_EQ(LUA_TNUMBER, lua_type(L, -1));
    ASSERT_EQ(3, lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_pushnumber(L, 2);
    lua_gettable(L, -2);
    ASSERT_EQ(LUA_TNUMBER, lua_type(L, -1));
    ASSERT_EQ(4, lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_pushstring(L, "key2");
    lua_gettable(L, -2);
    ASSERT_EQ(LUA_TNUMBER, lua_type(L, -1));
    ASSERT_EQ(5, lua_tonumber(L, -1));
    lua_pop(L, 1);

    lua_pop(L, 1);
}

static void RandomString(char* s, int max_len)
{
    int n = rand() % max_len + 1;
    for (int i = 0; i < n; ++i)
    {
        (*s++) = (char)(rand() % 256);
    }
    *s = '\0';
}

TEST_F(LuaTableTest, Stress)
{
    accept_panic = true;

    for (int iter = 0; iter < 100; ++iter)
    {
        for (int buf_size = 0; buf_size < 256; ++buf_size)
        {
            int n = rand() % 15 + 1;

            lua_newtable(L);
            for (int i = 0; i < n; ++i)
            {
                int key_type = rand() % 2;
                if (key_type == 0)
                {
                    char key[12];
                    RandomString(key, 11);
                    lua_pushstring(L, key);
                }
                else if (key_type == 1)
                {
                    lua_pushnumber(L, rand() % (n + 1));
                }
                int value_type = rand() % 3;
                if (value_type == 0)
                {
                    lua_pushboolean(L, 1);
                }
                else if (value_type == 1)
                {
                    lua_pushnumber(L, 123);
                }
                else if (value_type == 2)
                {
                    char value[16];
                    RandomString(value, 15);
                    lua_pushstring(L, value);
                }

                lua_settable(L, -3);
            }
            char* buf = new char[buf_size];

            bool check_ok = false;
            int ret = setjmp(env);
            if (ret == 0)
            {
                uint32_t buffer_used = dmScript::CheckTable(L, buf, buf_size, -1);
                check_ok = true;
                (void) buffer_used;


                dmScript::PushTable(L, buf);
                lua_pop(L, 1);
            }
            lua_pop(L, 1);

            delete[] buf;
        }
    }
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}
