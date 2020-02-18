#include "audio.hpp"
#include "string.hpp"

#include <string>
#include <map>

namespace genie {

const std::map<std::string, SfxId> taunts{
	{"1", SfxId::taunt1}, {"yes", SfxId::taunt1}, // yes
	{"2", SfxId::taunt2}, {"no", SfxId::taunt2}, // no
	{"3", SfxId::taunt3}, {"food", SfxId::taunt3}, {"i need food", SfxId::taunt3}, {"food please", SfxId::taunt3}, // i need food
	{"4", SfxId::taunt4}, {"wood", SfxId::taunt4}, {"i need wood", SfxId::taunt4}, {"wood please", SfxId::taunt4}, // somebody pass the wood
	{"5", SfxId::taunt5}, {"gold", SfxId::taunt5}, {"i need gold", SfxId::taunt5}, {"gold please", SfxId::taunt5}, // gold please
	{"6", SfxId::taunt6}, {"stone", SfxId::taunt6}, {"i need stone", SfxId::taunt6}, {"stone please", SfxId::taunt6}, // gimme some stone
	{"7", SfxId::taunt7}, {"eh", SfxId::taunt7}, {"ehh", SfxId::taunt7}, // hehhh
	{"8", SfxId::taunt8}, {"futile", SfxId::taunt8}, {"your attempts are futile", SfxId::taunt8}, {"your attempt is futile", SfxId::taunt8}, // your attempts are futile
	{"9", SfxId::taunt9}, {"yahoo", SfxId::taunt9}, {"yeeahoo", SfxId::taunt9}, // yeeahoo hahaha yeeahoo hahaha whew
	{"10", SfxId::taunt10}, {"town", SfxId::taunt10}, {"i'm in your town", SfxId::taunt10}, // hey, i'm in your town
	{"11", SfxId::taunt11}, {"uwaaahh", SfxId::taunt11}, // uwaaahh
	{"12", SfxId::taunt12}, {"join", SfxId::taunt12}, {"join me", SfxId::taunt12}, // join me
	{"13", SfxId::taunt13}, {"nope", SfxId::taunt13}, {"i don't think so", SfxId::taunt13}, // i don't think so
	{"14", SfxId::taunt14}, {"start", SfxId::taunt14}, {"start already", SfxId::taunt14}, {"please start", SfxId::taunt14}, {"start please", SfxId::taunt14}, // start the game already
	{"15", SfxId::taunt15}, {"who's the man", SfxId::taunt15}, // who's the man
	{"16", SfxId::taunt16}, {"attack", SfxId::taunt16}, // attack them now
	{"17", SfxId::taunt17}, {"haha", SfxId::taunt17}, // hahahaah
	{"18", SfxId::taunt18}, {"i'm weak", SfxId::taunt18}, {"spare me", SfxId::taunt18}, {"have mercy", SfxId::taunt18}, // i'm weak, please don't kill me
	{"19", SfxId::taunt19}, {"lol", SfxId::taunt19}, {"hahaha", SfxId::taunt19}, // hahahahahaha
	{"20", SfxId::taunt20}, {"satisfying", SfxId::taunt20}, // i just got some... satisfaction
	{"21", SfxId::taunt21}, {"nice town", SfxId::taunt21}, // hey, nice town
	{"22", SfxId::taunt22}, {"wtf", SfxId::taunt22}, {"wut", SfxId::taunt22}, {"don't mess with me", SfxId::taunt23}, // we will not tolerate this behavior
	{"23", SfxId::taunt23}, {"get out", SfxId::taunt23}, {"screw you", SfxId::taunt23}, // get out
	{"24", SfxId::taunt24}, // that gom????
	{"25", SfxId::taunt25}, {"yeah", SfxId::taunt25}, // oh yeah
	{"26", SfxId::priest_converted}, {"hayo", SfxId::priest_converted}, // hayoo
	{"27", SfxId::priest_convert1}, {"hayoo", SfxId::priest_convert1}, // hayooooh ...
	{"28", SfxId::priest_convert2}, {"wololo", SfxId::priest_convert2}, // wololo
	{"29", SfxId::priest_convert_reverse}, {"ololow", SfxId::priest_convert_reverse},
};

void check_taunt(const std::string &str) {
	auto s = trim_copy(str);
	tolower(s);

	// check if a taunt has been sent
	trim(s);
	tolower(s);

	auto search = taunts.find(s);
	if (search != taunts.end())
		jukebox.sfx(search->second);
}

}