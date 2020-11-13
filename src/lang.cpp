#include "lang.hpp"

#include <cassert>

#include <vector>
#include <string>

#include "imgui/imgui.h"

#include <SDL2/SDL_version.h>

// NOTE: never load TXT and TXTF data externally as this enables remote code execution in vspnrintf
// NOTE: always ensure that any translation in an imgui window is unique. if it isn't you can suffix it with ## as everything after ## is not shown on screen

extern const char *langs[] = {
	"English",
	//"Chinese", // 中文
	"Dutch", // Nederlands
	"German", // Deutsch
	//"Japanese", // 日本語
	//"Spanish", // Español
	//"Turkish", // Türk
	//"Vietnamese", // Tiếng Việt
	NULL
};

enum LangId lang = LangId::standard;

#define str(x) #x
#define stre(x) str(x)

static std::vector<std::vector<std::string>> txts = {
	// English
	{
		"Age of Empires Remake",
		"Language",
		"Welcome to the free software remake of Age of Empires. This project is not affiliated with Microsoft or Ensemble Studios in any way. If you have paid for this remake edition, you have been ripped off.\n\nTo get started, you need to have the original game installed from CD-ROM to get this to work. The Definitive Edition on Steam is not supported yet.",
		"About free software Age of Empires\nversion 0"
			"\n\n"
			"Libraries:\n"
			"  Dear ImGui version " IMGUI_VERSION " Copyright (c) 2014-2020 Omar Cornut and others\n"
			"  Simple DirectMedia Layer " stre(SDL_MAJOR_VERSION) "." stre(SDL_MINOR_VERSION) " Copyright(c) 1998-2020 Sam Lantinga"
			"  miniz compression library v115 r4 Copyright (c) 2011-2013 Rich Geldreich", // https://code.google.com/archive/p/miniz/
		"Set game directory",
		"Choose installation directory of original game",
		"Global settings",
		// buttons
		"Quit",
		"Back",
		"About",
		"Start game",
		// main menu
		"Single player",
		"Multiplayer",
		"Settings",
		"Editor",
		// editor
		"Scenario Editor",
		"Campaign Editor",
		"DRS Viewer",
		// scn editor
		"Load",
		// settings
		"Sound",
		"Sound volume",
		"Music",
		"Music volume",
		"Fullscreen",
		"Center",
		"Display mode",
		"Custom",
		// tasks
		"Game configured successfully",
		"Looking for data resource sets",
		"Loading...",
		// errors
		"Cannot find data resource set \"%s\"",
		"Bad DRS file \"%s\": invalid header",
		"DRS file \"%s\" must have at least one list",
		"Could not find DRS item %u",
		"Could not find TTF font \"%s\"",
	},
	// Chinese
	//{},
	// Nederlands
	{
		"Age of Empires Namaak",
		"Taal",
		"Welkom bij de free software Age of Empires Namaak-editie. Dit project is op geen enkele manier geassocieerd met Microsoft of Ensemble Studios. Als u betaald hebt voor deze namaak-editie, dan bent u belazerd en bedonderd.\n\nOm het spel te spelen heeft u de originele CD-ROM versie nodig. De Definitive Edition op Steam wordt nog niet ondersteund.",
		"Over free software Age of Empires Namaak-editie",
		"Kies spelmap",
		"Kies installatiemap van origineel spel",
		"Globale instellingen",
		// knopjes
		"Stop",
		"Terug",
		"Over",
		"Speel",
		// hoofdmenu
		"Alleen spelen",
		"Samen spelen",
		"Instellingen",
		"Editor",
		// editor
		"Scenario bewerken",
		"Campagne bewerken",
		"DRS bekijken",
		// scn bewerker
		"Laad",
		// instellingen
		"Geluid",
		"Geluidsvolume",
		"Muziek",
		"Muzieksvolume",
		"",
		"Centreer",
		"Schermmodus",
		"Anders",
		// taken
		"Spelconfiguratie voltooid",
		"Data resource sets zoeken",
		"Laden...",
		// foutmeldingen
		"Data resource set \"%s\" kan niet gevonden worden",
		"Fout in DRS \"%s\": onjuiste header",
		"Fout in DRS \"%s\": geen lijsten",
		"DRS nummer %u niet gevonden",
		"Kan TTF font \"%s\" niet vinden",
	},
	// Deutsch
	{
		"Age of Empires Remake",
		"Sprache",
		"",//"Willkommen zum kostenlosen Software-Remake Age of Empires. Dieses Projekt ist in keiner Weise mit Microsoft oder Ensemble Studios verbunden. Wenn Sie für diese Remake-Edition bezahlt haben, wurden Sie abgezockt.\n\nUm loszulegen, muss das Originalspiel von der CD-ROM installiert sein, damit dies funktioniert. Die Definitive Edition auf Steam wird noch nicht unterstützt.",
		"",
		"Spielverzeichnis einstellen",
		"Installationsverzeichnis des Originalspiels einstellen",
		"",
		// Klicktasten
		"Stop",
		"Zurück",
		"Over",
		"Spiel",
		"",
		"",
		"Einstellung",
		"Editor",
		// Editor
		"",
		"",
		"",
		// Scenario Editor
		"",
		// Einstellung
		"Audio",
		"Audiovolume",
		"Musik",
		"Musikvolume",
		"",
		"",
		"Bildmodus",
		"Anders",
		// Taken
		"Konfiguration erfüllt",
		"Datenressourcensätzen suchen",
		"Laden...",
		// Fehlers
		"Datenressourcensatz \"%s\" kann nicht gefunden werden",
		"Beschädigte DRS-Datei \"%s\": ungültiger Header",
		"Beschädigte DRS-Datei \"%s\": leere Liste",
		"DRS nummer %u nicht gefunden",
		"TTF Font \"%s\" nicht gefunden",
	},
	// 日本語
	//{},
	// Español
	//{},
	// Türk
	//{},
	// Tiếng Việt
	//{},
};

const std::string &TXT(LangId lang, TextId id) {
	assert(txts[0].size() == size_t(TextId::max));
	assert(txts.size() == size_t(LangId::max));

	return size_t(id) >= txts[size_t(lang)].size() || txts[size_t(lang)][size_t(id)].length() == 0 ? txts[0][size_t(id)] : txts[size_t(lang)][size_t(id)];
}

std::string TXTF(LangId lang, TextId id, ...)
{
	va_list args;
	va_start(args, id);
	std::string s(TXTF(lang, id, args));
	va_end(args);
	return s;
}

std::string TXTF(LangId lang, TextId id, va_list args)
{
	const std::string &fmt = TXT(lang, id);
	va_list copy;
	va_copy(copy, args);
	int need = vsnprintf(NULL, 0, fmt.c_str(), copy);
	va_end(copy);

	if (need <= 0)
		return "???";

	std::string s(need, 0);
	// s.size() + 1 abuses the fact that c++11 strings always have a null terminator byte
	vsnprintf(s.data(), s.size() + 1, fmt.c_str(), args);
	return s;
}
