#pragma once

#include "../../lua/lua.hpp"

#include "core/game.h"
#include "core/adventure_map.h"

//probably need to change this
namespace lua {
	//global/map-wide functions
	int GetCurrentDay(lua_State* L);
	int GetCurrentWeek(lua_State* L);
	int GetCurrentMonth(lua_State* L);
	int GetPlayerCount(lua_State* L);
	int GetCurrentPlayer(lua_State* L);

	//hero-related functions
	int SetHeroName(lua_State* L);
	int SetHeroClass(lua_State* L);
	int SetHeroLevel(lua_State* L);
	int AddHeroExperience(lua_State* L);
	int LearnSkill(lua_State* L);
	int UnlearnSkill(lua_State* L);
	int UpgradeSkill(lua_State* L);
	int GiveHeroArtifact(lua_State* L);


	//player-specific functions
	int SendPlayerConsoleMessage(lua_State* L);
	int GivePlayerResources(lua_State* L);
	int ShowPlayerMessage(lua_State* L);

	//used to signal when a script's condition has triggered and it no lnoger needs to be run
	int ScriptCompleted(lua_State* L);
};

struct lua_function_info_t {
	int (*fptr)(lua_State*);
	QString name;
	int arguments;
	QString documentation;
};

//need to change this
extern QList<lua_function_info_t> lua_functions;
const QList<lua_function_info_t>& get_lua_functions();

int register_lua_functions(lua_State* L, game_t* game_ptr);
void update_lua_hero_table(lua_State* L, hero_t* hero_ptr);
