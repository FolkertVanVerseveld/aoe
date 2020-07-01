#include "lang.hpp"

#include <cassert>

#include <vector>
#include <string>

#include "imgui/imgui.h"

// NOTE: never load TXT and TXTF data externally as this enables remote code execution in vspnrintf
// NOTE: always ensure that any translation in a imgui window is unique. if it isn't you can suffix it with ## as everything after ## is not shown on screen

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

static std::vector<std::vector<const char*>> txts = {
	// English
	{
		"Age of Empires Remake",
		"Language",
		"Welcome to the free software remake of Age of Empires. This project is not affiliated with Microsoft or Ensemble Studios in any way. If you have paid for this remake edition, you have been ripped off.\n\nTo get started, you need to have the original game installed from CD-ROM to get this to work. The Definitive Edition on Steam is not supported yet.",
		"About free software Age of Empires\nversion 0\n\nImGui version " IMGUI_VERSION,
		"Set game directory",
		"Choose installation directory of original game",
		// buttons
		"Quit",
		"Back",
		"About",
		"Start game",
		"Editor",
		// tasks
		"Game configured successfully",
		"Looking for data resource sets",
		// errors
		"Cannot find data resource set \"%s\"",
		"Bad DRS file \"%s\": invalid header",
		"Could not find DRS item %u",
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
		// knopjes
		"Stop",
		"Terug",
		"Over",
		"Speel",
		"Editor",
		// taken
		"Spelconfiguratie voltooid",
		"Data resource sets zoeken",
		// foutmeldingen
		"Data resource set \"%s\" kan niet gevonden worden",
		"Fout in DRS \"%s\": onjuiste header",
		"DRS nummer %u niet gevonden",
	},
	// Deutsch
	{
		"Age of Empires Remake",
		"Sprache",
		NULL,//"Willkommen zum kostenlosen Software-Remake Age of Empires. Dieses Projekt ist in keiner Weise mit Microsoft oder Ensemble Studios verbunden. Wenn Sie für diese Remake-Edition bezahlt haben, wurden Sie abgezockt.\n\nUm loszulegen, muss das Originalspiel von der CD-ROM installiert sein, damit dies funktioniert. Die Definitive Edition auf Steam wird noch nicht unterstützt.",
		NULL,
		"Spielverzeichnis einstellen",
		"Installationsverzeichnis des Originalspiels einstellen",
		// Klicktasten
		"Stop",
		"Zurück",
		"Over",
		"Spiel",
		"Editor",
		// Taken
		"Konfiguration erfüllt",
		"Datenressourcensätzen suchen",
		// Fehlers
		"Datenressourcensatz \"%s\" kann nicht gefunden werden",
		"Beschädigte DRS-Datei \"%s\": ungültiger Header",
		"DRS nummer %u nicht gefunden"
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

const char *TXT(LangId lang, TextId id) {
	assert(txts[0].size() == size_t(TextId::max));
	assert(txts.size() == size_t(LangId::max));

	return size_t(id) >= txts[size_t(lang)].size() || !txts[size_t(lang)][size_t(id)] ? txts[0][size_t(id)] : txts[size_t(lang)][size_t(id)];
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
	const char *fmt = TXT(lang, id);
	va_list copy;
	va_copy(copy, args);
	int need = vsnprintf(NULL, 0, fmt, copy);
	va_end(copy);

	if (need <= 0)
		return "???";

	std::string s(need, 0);
	// s.size() + 1 abuses the fact that c++11 strings always have a null terminator byte
	vsnprintf(s.data(), s.size() + 1, fmt, args);
	return s;
}
