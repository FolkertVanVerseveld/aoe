/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * PE resource wrapper API
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 * Convient API for reading PE resources
 */

#ifndef RES_H
#define RES_H

#include <libpe/pe.h>

void free_nodes(NODE_PERES *node);
NODE_PERES *createNode(NODE_PERES *currentNode, NODE_TYPE_PERES typeOfNextNode);
const NODE_PERES *lastNodeByTypeAndLevel(const NODE_PERES *currentNode, NODE_TYPE_PERES nodeTypeSearch, NODE_LEVEL_PERES nodeLevelSearch);
NODE_PERES *pe_map_res(pe_ctx_t *ctx);

#endif
