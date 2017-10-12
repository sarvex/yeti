//===-- yeti/script.cc ----------------------------------*- mode: C++11 -*-===//
//
//                             __ __     _   _
//                            |  |  |___| |_|_|
//                            |_   _| -_|  _| |
//                              |_| |___|_| |_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#include "yeti/script.h"
#include "yeti/script/environment.h"

#include "yeti/resource_manager.h"

#include "yeti/resources/script_resource.h"

// TODO(mtwilliams): Support Lua 5.2 by using LUA_REGISTRYINDEX and LUA_RIDX_GLOBALS.

// Stolen from the Lua compatability project.
#if LUA_VERSION_NUM <= 501
  extern "C" {
    static int lua_absindex (lua_State *L, int i) {
      if (i < 0 && i > LUA_REGISTRYINDEX)
        i += lua_gettop(L) + 1;
      return i;
    }

    static int luaL_getsubtable(lua_State *L, int i, const char *name) {
      int abs_i = lua_absindex(L, i);
      lua_pushstring(L, name);
      lua_gettable(L, abs_i);
      if (lua_istable(L, -1))
        return 1;
      lua_pop(L, 1);
      lua_newtable(L);
      lua_pushstring(L, name);
      lua_pushvalue(L, -2);
      lua_settable(L, abs_i);
      return 0;
    }

    static void luaL_requiref(lua_State *L, const char *modname,
                              lua_CFunction openf, int glb) {
      luaL_getsubtable(L, LUA_REGISTRYINDEX, "_LOADED");
      if ((lua_getfield(L, -1, modname), lua_type(L, -1)) == LUA_TNIL) {
        lua_pop(L, 1);
        lua_pushcfunction(L, openf);
        lua_pushstring(L, modname);
        lua_call(L, 1, 1);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, modname);
      }
      if (glb) {
        lua_pushvalue(L, -1);
        lua_setglobal(L, modname);
      }
      lua_replace(L, -2);
    }
  }
#endif

namespace yeti {

Script::Script() {
#if YETI_ARCHITECTURE == YETI_ARCHITECTURE_X86
  L = lua_newstate((lua_Alloc)&Script::__alloc, (void *)this);
#elif YETI_ARCHITECTURE == YETI_ARCHITECTURE_X86_64
  // TODO(mtwilliams): Use our own allocator for Lua.
  L = luaL_newstate();
#endif

  // Insert reference to aid recovery. See `Script::recover`.
  lua_createtable(L, 0, 1);
  lua_pushlightuserdata(L, (void *)this);
  lua_setfield(L, -2, "__instance__");
  lua_setglobal(L, "Script");

  static const luaL_Reg libs[] = {
    { "",              luaopen_base     },
    { LUA_LOADLIBNAME, &luaopen_package },
    { LUA_TABLIBNAME,  &luaopen_table   },
    { LUA_STRLIBNAME,  &luaopen_string  },
    { LUA_MATHLIBNAME, &luaopen_math    },
    { LUA_DBLIBNAME,   &luaopen_debug   },
    { NULL,            NULL             }
  };

  // Register safe libraries.
  for (const luaL_Reg *lib = &libs[0]; lib->func; ++lib) {
    luaL_requiref(L, lib->name, lib->func, 1);
    lua_pop(L, 1);
  }

  // Use our own loader.
  lua_getglobal(L, "package");
  lua_createtable(L, 1, 0);
  lua_pushlightuserdata(L, (void *)this);
  lua_pushcclosure(L, &Script::__require, 1);
  lua_rawseti(L, -2, 1);
  lua_setfield(L, -2, "loaders");
  lua_pop(L, 1);

  // Allocate environment separately in case this ends up on the stack.
  E = YETI_NEW(ScriptEnvironment, core::global_heap_allocator());
}

Script::~Script() {
  lua_close(L);

  // Destroy environment after closing virtual machine, just in case the
  // environment is touched when closing.
  YETI_DELETE(ScriptEnvironment, core::global_heap_allocator(), E);
}

void *Script::__alloc(Script *script, void *ptr, size_t, size_t size) {
  return core::global_heap_allocator().reallocate(ptr, size);
}

int Script::__require(lua_State *L) {
  const char *script_name = luaL_checkstring(L, 1);

  static const Resource::Type *script_resource_type =
    resource::type_from_name("script");

  static const Resource::Type::Id script_resource_type_id =
    resource::id_from_type(script_resource_type);

  const Resource::Id script_id =
    resource::id_from_name(script_resource_type_id, script_name);

  ScriptResource *script_resource =
    (ScriptResource *)resource_manager::lookup(script_id);

  if (resource_manager::autoloads()) {
    // Wait until resource is loaded.
    while (resource_manager::state(script_id) != Resource::LOADED)
      core::Thread::yield();
  } else {
    // Ensure resource is loaded.
    if (!resource_manager::available(script_id))
      return 0;
  }

  luaL_loadbuffer(L, (const char *)script_resource->bytecode_, script_resource->bytecode_len_, script_resource->path_);

  script_resource->deref();

  return 1;
}

int Script::__error_handler(lua_State *L) {
  Script *script = (Script *)lua_touserdata(L, lua_upvalueindex(1));

  // TODO(mtwilliams): Forward to a user-defined error handler.
  // TODO(mtwilliams): Use lua_Debug to build a Lua & C/C++ callstack.

  core::logf(core::log::SCRIPT, core::log::ERROR, lua_tostring(L, -1));

  return 0;
}

Script *Script::recover(lua_State *L) {
  yeti_assert_debug(L != NULL);

  lua_getglobal(L, "Script");
  lua_getfield(L, -1, "__instance__");

  yeti_assert_with_reason_debug(lua_islightuserdata(L, -1),
                                "Expected Script.__instance__ to be a light user-data reference to a `Script`.");

  Script *script = (Script *)lua_touserdata(L, -1);

  lua_pop(L, 2);

  return script;
}

void Script::inject(const ScriptResource *script_resource) {
  yeti_assert_debug(script_resource != NULL);

  lua_pushlightuserdata(L, (void *)this);
  lua_pushcclosure(L, &Script::__error_handler, 1);

  if (luaL_loadbuffer(L, (const char *)script_resource->bytecode_, script_resource->bytecode_len_, script_resource->path_) != 0)
    yeti_assert_with_reason(0, "Script injection failed!\n > %s", lua_tostring(L, -1));

  if (lua_pcall(L, 0, 0, -2) != 0)
    yeti_assert_with_reason(0, "Script injection failed!\n > %s", lua_tostring(L, -1));
}

void Script::add_function(const char *name,
                          const lua_CFunction fn) {
  lua_pushcfunction(L, fn);
  lua_setglobal(L, name);
}

void Script::add_module(const char *module) {
  lua_newtable(L);
  lua_setglobal(L, module);
}

void Script::add_module_constructor(const char *module,
                                    const lua_CFunction fn) {
  lua_getglobal(L, module);

  lua_createtable(L, 0, 1);
  lua_pushcfunction(L, fn);
  lua_setfield(L, -2, "__call");

  lua_setmetatable(L, -2);
}

void Script::add_module_function(const char *module,
                                 const char *name,
                                 const lua_CFunction fn) {
  lua_getglobal(L, module);
  lua_pushcfunction(L, fn);
  lua_setfield(L, -2, name);
  lua_pop(L, 1);
}

template <> bool Script::is_a<bool>(int index) {
  return !!lua_isboolean(L, index);
}

template <> bool Script::to_a<bool>(int index) {
  if (!lua_isboolean(L, index))
    luaL_typerror(L, index, "boolean");

  return !!lua_toboolean(L, index);
}

template <> bool Script::is_a<i8>(int index)  { return !!lua_isnumber(L, index); }
template <> bool Script::is_a<u8>(int index)  { return !!lua_isnumber(L, index); }
template <> bool Script::is_a<i16>(int index) { return !!lua_isnumber(L, index); }
template <> bool Script::is_a<u16>(int index) { return !!lua_isnumber(L, index); }
template <> bool Script::is_a<i32>(int index) { return !!lua_isnumber(L, index); }
template <> bool Script::is_a<u32>(int index) { return !!lua_isnumber(L, index); }
template <> bool Script::is_a<i64>(int index) { return !!lua_isnumber(L, index); }
template <> bool Script::is_a<u64>(int index) { return !!lua_isnumber(L, index); }
template <> bool Script::is_a<f32>(int index) { return !!lua_isnumber(L, index); }
template <> bool Script::is_a<f64>(int index) { return !!lua_isnumber(L, index); }

template <> i8 Script::to_a<i8>(int index) { return (i8)luaL_checknumber(L, index); }
template <> u8 Script::to_a<u8>(int index) { return (u8)luaL_checknumber(L, index); }
template <> i16 Script::to_a<i16>(int index) { return (i16)luaL_checknumber(L, index); }
template <> u16 Script::to_a<u16>(int index) { return (u16)luaL_checknumber(L, index); }
template <> i32 Script::to_a<i32>(int index) { return (i32)luaL_checknumber(L, index); }
template <> u32 Script::to_a<u32>(int index) { return (u32)luaL_checknumber(L, index); }
template <> i64 Script::to_a<i64>(int index) { return (i64)luaL_checknumber(L, index); }
template <> u64 Script::to_a<u64>(int index) { return (u64)luaL_checknumber(L, index); }

template <> bool Script::is_a<const char *>(int index) {
  return !!lua_isstring(L, index);
}

template <> const char *Script::to_a<const char *>(int index) {
  return luaL_checkstring(L, index);
}

template <> bool Script::is_a<Script::Reference>(int index) {
  if (!lua_islightuserdata(L, index))
    return false;

  const uintptr_t lud = lua_islightuserdata(L, index);

  return ((lud & 0x15) == 0x15);
}

template <> Script::Reference Script::to_a<Script::Reference>(int index) {
  if (!lua_islightuserdata(L, index))
    luaL_typerror(L, index, "ref");

  const uintptr_t lud = lua_islightuserdata(L, index);

  if ((lud & 0x15) != 0x15)
    luaL_typerror(L, index, "ref");

  Script::Reference reference;
  reference.opaque = (lud >> 4) & 0xFFFFFFFF;
  return reference;
}

template <> void Script::push<Script::Reference>(Script::Reference reference) {
  const uintptr_t lud = ((uintptr_t)reference.opaque << 4) | 0x15;
  lua_pushlightuserdata(L, (void *)lud);
}

template <> bool Script::is_a<Vec2>(int index) {
  return E->valid<Vec2>((Vec2 *)lua_touserdata(L, index));
}

template <> Vec2 Script::to_a<Vec2>(int index) {
  const Vec2 *temporary = (const Vec2 *)lua_touserdata(L, index);

  if (!E->valid<Vec2>(temporary))
    luaL_typerror(L, index, "Vec2");

  return *temporary;
}

template <> void Script::push<Vec2>(Vec2 v) {
  Vec2 *storage = E->allocate<Vec2>();
  *storage = v;
  lua_pushlightuserdata(L, (void *)storage);
}

template <> bool Script::is_a<Vec3>(int index) {
  return E->valid<Vec3>((Vec3 *)lua_touserdata(L, index));
}

template <> Vec3 Script::to_a<Vec3>(int index) {
  const Vec3 *temporary = (const Vec3 *)lua_touserdata(L, index);

  if (!E->valid<Vec3>(temporary))
    luaL_typerror(L, index, "Vec3");

  return *temporary;
}

template <> void Script::push<Vec3>(Vec3 v) {
  Vec3 *storage = E->allocate<Vec3>();
  *storage = v;
  lua_pushlightuserdata(L, (void *)storage);
}

template <> bool Script::is_a<Vec4>(int index) {
  return E->valid<Vec4>((Vec4 *)lua_touserdata(L, index));
}

template <> Vec4 Script::to_a<Vec4>(int index) {
  const Vec4 *temporary = (const Vec4 *)lua_touserdata(L, index);

  if (!E->valid<Vec4>(temporary))
    luaL_typerror(L, index, "Vec4");

  return *temporary;
}

template <> void Script::push<Vec4>(Vec4 v) {
  Vec4 *storage = E->allocate<Vec4>();
  *storage = v;
  lua_pushlightuserdata(L, (void *)storage);
}

template <> bool Script::is_a<Quaternion>(int index) {
  return E->valid<Quaternion>((Quaternion *)lua_touserdata(L, index));
}

template <> Quaternion Script::to_a<Quaternion>(int index) {
  const Quaternion *temporary = (const Quaternion *)lua_touserdata(L, index);

  if (!E->valid<Quaternion>(temporary))
    luaL_typerror(L, index, "Quaternion");

  return *temporary;
}

template <> void Script::push<Quaternion>(Quaternion v) {
  Quaternion *storage = E.allocate<Quaternion>();
  *storage = v;
  lua_pushlightuserdata(L, (void *)storage);
}

bool Script::call(const char *fn, u32 n, ...) {
  lua_pushlightuserdata(L, (void *)this);
  lua_pushcclosure(L, &Script::__error_handler, 1);

  lua_getglobal(L, fn);

  va_list ap;
  va_start(ap, n);

  for (u32 arg = 0; arg < n; ++arg) {
    const Type type = (Type)va_arg(ap, int);
    switch (type) {
      case T_NIL:      lua_pushnil(L); break;
      case T_BOOLEAN:  lua_pushboolean(L, va_arg(ap, int)); break;
      case T_INTEGER:  lua_pushnumber(L, va_arg(ap, int)); break;
      case T_FLOAT:    lua_pushnumber(L, va_arg(ap, double)); break;
      case T_STRING:   lua_pushstring(L, va_arg(ap, const char *)); break;
      case T_ARRAY:    yeti_assert_with_reason(0, "Not implemented yet."); break;
      case T_MAP:      yeti_assert_with_reason(0, "Not implemented yet."); break;
      case T_POINTER:  lua_pushlightuserdata(L, va_arg(ap, void *)); break;
      case T_FUNCTION: lua_pushcfunction(L, (lua_CFunction)va_arg(ap, void *)); break;
      default:         yeti_assert_with_reason(0, "Invalid or unknown type."); break;
    }
  }

  va_end(ap);

  if (lua_pcall(L, n, 0, -(int)(n + 2)) != 0) {
    lua_pop(L, 1 + 1);
    return false;
  } else {
    lua_pop(L, 1);
  }

  return true;
}

} // yeti
