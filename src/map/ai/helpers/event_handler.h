/*
===========================================================================

Copyright (c) 2010-2015 Darkstar Dev Teams

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see http://www.gnu.org/licenses/

This file is part of DarkStar-server source code.

===========================================================================
*/

#ifndef _EVENT_HANDLER
#define _EVENT_HANDLER

#include <map>
#include <vector>
#include <functional>

#include "../../../common/cbasetypes.h"
#include "../../lua/luautils.h"

struct ai_event_t
{
    std::string identifier;
    int lua_func;

    ai_event_t(std::string _ident, int _lua_func) : 
        identifier(_ident), lua_func(_lua_func) {}
};

class CAIEventHandler
{
public:
    void addListener(std::string eventname, int lua_func, std::string identifier = "");
    void removeListener(std::string eventname, std::string identifier = "");

    // calls event from core
    template<class... Types>
    void triggerListener(std::string& eventname, Types&&... args)
    {
        for (auto&& event : eventListeners[eventname])
        {
            int nargs = sizeof...(args);
            luautils::pushFunc(event.lua_func);
            pushArg(std::forward<Types&&...>(args)...);
            luautils::callFunc(nargs);
        }
    }
    //calls event from lua
    void triggerListener(std::string eventname, int nargs)
    {
        for (auto&& event : eventListeners[eventname])
        {
            luautils::pushFunc(event.lua_func, nargs);
            luautils::callFunc(nargs);
        }
    }

private:
    std::map<std::string, std::vector<ai_event_t>> eventListeners;

    // push parameters on lua stack
    template<class T>
    void pushArg(T&& arg) { luautils::pushArg(std::forward<T>(arg)); }
    template<class T, class... Types>
    void pushArg(T&& arg, Types&&... args) 
    {
        pushArg(std::forward<T>(arg));
        pushArg(std::forward<Types&&...>(args)...);
    }
};

#endif