/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "scenario.hpp"
#include "fs.hpp"

#include "../setup/def.h"

#include <string.h>

Campaign::Campaign(const char *path) : path(path) {
	cpn_init(&cpn);
	int err;

	// Just skim campaign for scenarios
	FileBlob file(path);
	if ((err = cpn_read(&cpn, file.blob.data, file.blob.size)))
		panicf("bad campaign: code %d\n", err);
	if (!(data = malloc(size = cpn_list_size(&cpn))))
		panic("bad campaign or out of memory");

	// Copy list of scenarios and close it
	memcpy(data, file.blob.data, size);
	cpn_read(&cpn, data, size);
	cpn.hdr->name[CPN_HDR_NAME_MAX - 1] = '\0';
	name = cpn.hdr->name;
}

Campaign::~Campaign() {
	free(data);
	cpn_free(&cpn);
}

const struct cpn_scn *Campaign::scenarios(size_t &count) const {
	count = cpn.hdr->scenario_count;
	return cpn.scenarios;
}
