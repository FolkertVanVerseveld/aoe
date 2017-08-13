/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Genie game engine system command interface
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 * This should not be confused with the in-game shell. This interface
 * wraps the system(3) call for debugging purposes.
 */

#ifndef GENIE_SYSTEM_H
#define GENIE_SYSTEM_H

int genie_system(const char *command);

#endif
