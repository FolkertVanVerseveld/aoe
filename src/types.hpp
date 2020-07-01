/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

/*
 * Declare some types that are problematic to include in a particular file because
 * it has multiple dependencies and abstracting these will remedy this problem.
 */

#include <cstdint>

namespace genie {

// NOTE both types are just uint16_t, because it doesn't make sense to have more than 65535 players

/**
 * Virtual unique mapping of connected slave to in-game player identifier.
 * Note that many user_ids may control the same player_id. This is useful when an OP wants
 * to take control of another player.
 */
typedef uint16_t player_id;
/**
 * Direct identifier mapping for connected slave (i.e. socket connected to serversocket).
 * E.g. client A connects to server as fd 5 (or 204 on windows), the server generates a
 * unique user_id that acts like the direct identifier for any related network data that
 * becomes available.
 */
typedef uint16_t user_id;

typedef uint16_t res_id;

}
