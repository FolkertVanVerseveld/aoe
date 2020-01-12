#pragma once

/**
 * True Type Font rendering subsystem
 *
 * This is more complicated than one would expect...
 * Since libfreetype does not properly render any TTF type I throw at it,
 * we will use windows native GDI rendering instead to improve font quality.
 */

class TTF final {
public:
	TTF();
	~TTF();
};