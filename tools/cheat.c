/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <stdio.h>
#include <string.h>

/*
clue: str_weird_convert works for both azerty and qwerty?

  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
0 @  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O
1 P  Q  R  S  T  U  V  W  X  Y  Z  [  \  ]  ^  _

0 @  A  B  C   D    E  F  G  H  I  J  K  L  M  N  O
1 P  Q  R  S   T    U  V  W  X  Y  Z  [  \  ]  ^  _
0 1- xh _* C3 >?   mA mJ Z1 )@ L# {} Wc !+ c% Wd ^%
1 >w xh %% $1 '^ ' j_ T& 34 () :: ?S K& yR X7 i; &*
*/
static void str_weird_convert(const char *str, char *dest, int n)
{
	const int strtbl[] = {
		//   @,      A,      B,      C,      D,      E,      F,      G
		//  1-,     xh,     _*,     C3,     >?,     mA,     mJ,     Z1
		0x2d31, 0x6878, 0x2a5f, 0x3343, 0x3f3e, 0x416d, 0x4a6d, 0x315a,
		//   H,      I,      J,      K,      L,      M,      N,      O
		//  )@,     L#,     {},     Wc,     !+,     c%,     Wd,     ^%
		0x4029, 0x234c, 0x7d7b, 0x6357, 0x2b21, 0x2563, 0x6457, 0x255e,
		//   P,      Q,      R,      S,      T,      U,      V,      W
		//  >w,     xh,     %%,     $1,     ^ ,     j_,     T&,     34
		0x773e, 0x6878, 0x2525, 0x3124, 0x205e, 0x5f6a, 0x2654, 0x3433,
		//   X,      Y,      Z,      [,      \,      ],      ^,      _
		//  (),     ::,     ?S,     K&,     yR,     X7,     i;,     &*
		0x2928, 0x3a3a, 0x533f, 0x264b, 0x5279, 0x3758, 0x3b69, 0x2a26
	};
	char *result;
	int ch, i, index;
	for (i = index = 0, result = dest; (ch = str[i]) != '\0'; ++i) {
		if (index >= n)
			break;
		if (ch < 'A' || ch > '_') {
			*result++ = ch;
			++index;
		} else {
			int item = strtbl[ch - '@'];
			*result++ = item >> 8;
			*result++ = item & 0xff;
			index += 2;
		}
	}
	*result = '\0';
}

int main(void)
{
	char buf[256];
	const char *text[] = {
		"HOME RUN",
		// STORMBILLY
		// CONVERT THIS!
		// BIG MOMMA
		// POW
		// GRANTLINKSPENCE
		// KING ARTHUR
		"PHOTON MAN",
		"E=MC2 TROOPER",
		"JACK BE NIMBLE",
		"BIG BERTHA",
		"FLYING DUTCHMAN",
		"UPSIDFLINTMOBILE",
		"HOYOHOYO",
		"DARK RAIN",
		"BLACK RIDER",
		"MEDUSA",
		"ICBM",
		"DIEDIEDIE",
		"KILL1",
		"KILL2",
		"KILL3",
		"KILL4",
		"KILL5",
		"KILL6",
		"KILL7",
		"KILL8",
		"KILL9",
		"HARIKARI",
		"RESIGN",
		"GAIA",
		"PEPPERONI PIZZA",
		"COINAGE",
		"WOODSTOCK",
		"QUARRY",
		"STEROIDS",
		"BIGDADDY",
		"REVEAL MAP",
		"NO FOG",
		"ZEUS",
	}, *cheats[] = {
		"@)%^%cAm %%_jdW", // HOME RUN
		"w>@)%^ ^%^dW %chxdW", // PHOTON MAN
		"Am=%c3C2  ^%%%^%^w>Am%%", // E=MC2 TROOPER
		"}{hx3CcW *_Am dW#L%c*_+!Am", // JACK BE NIMBLE
		"*_#L1Z *_Am%% ^@)hx", // BIG BERTHA
		"Jm+!::#LdW1Z ?>_j ^3C@)%chxdW", // FLYING DUTCHMAN
		"_jw>1$#L?>Jm+!#LdW ^%c%^*_#L+!Am", // UPSIDFLINTMOBILE
		"@)%^::%^@)%^::%^", // HOYOHOYO
		"?>hx%%cW %%hx#LdW", // DARK RAIN
		"*_+!hx3CcW %%#L?>Am%%", // BLACK RIDER
		"%cAm?>_j1$hx", // MEDUSA
		"#L3C*_%c", // ICBM
		"?>#LAm?>#LAm?>#LAm", // DIEDIEDIE
		"cW#L+!+!1", // KILL1
		"cW#L+!+!2", // KILL2
		"cW#L+!+!3", // KILL3
		"cW#L+!+!4", // KILL4
		"cW#L+!+!5", // KILL5
		"cW#L+!+!6", // KILL6
		"cW#L+!+!7", // KILL7
		"cW#L+!+!8", // KILL8
		"cW#L+!+!9", // KILL9
		"@)hx%%#LcWhx%%#L", // HARIKARI
		"%%Am1$#L1ZdW", // RESIGN
		"1Zhx#Lhx", // GAIA
		"w>Amw>w>Am%%%^dW#L w>#LS?S?hx", // PEPPERONI PIZZA
		"3C%^#LdWhx1ZAm", // COINAGE
		"43%^%^?>1$ ^%^3CcW", // WOODSTOCK
		"hx_jhx%%%%::", // QUARRY
		"1$ ^Am%%%^#L?>1$", // STEROIDS
		"*_#L1Z?>hx?>?>::", // BIG DADDY
		"%%Am&TAmhx+! %chxw>", // REVEAL MAP
		"dW%^ Jm%^1Z", // NO FOG
		"S?Am_j1$", // ZEUS
	};
	#define TEXTSZ (sizeof text/sizeof(text[0]))
	#define CHEATSZ (sizeof cheats/sizeof(cheats[0]))
	if (TEXTSZ != CHEATSZ) {
		fprintf(stderr, "%lu %lu\n", TEXTSZ, CHEATSZ);
		return 1;
	}
	for (unsigned i = 0; i < TEXTSZ; ++i) {
		str_weird_convert(text[i], buf, sizeof buf);
		if (strcmp(buf, cheats[i])) {
			fprintf(stderr, "#%u failed:\n\"%s\"\n!=\n\"%s\"\n", i, buf, cheats[i]);
			return 1;
		}
	}
	return 0;
}
