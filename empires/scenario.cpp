/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "scenario.hpp"
#include "fs.hpp"

#include <genie/def.h>
#include "str.h"

#include <string.h>
#include <cinttypes>

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

Scenario::Scenario(const char *path) : path(path) {
	data = NULL;
	scn_init(&scn);
	int err;

	FileBlob file(path);
	if (!(data = malloc(size = file.blob.size)))
		panic("out of memory");
	memcpy(data, file.blob.data, size);
	if ((err = scn_read(&scn, data, size)))
		panicf("bad scenario: code %d\n", err);
	if ((err = scn_inflate(&scn)))
		panicf("bad scenario: code %d\n", err);
}

Scenario::~Scenario() {
	free(data);
	scn_free(&scn);
}

extern "C" struct scn_description *scn_get_description(const struct scn *s)
{
	return (struct scn_description*)((const unsigned char*)&s->hdr3[1] + s->hdr3->filename_length);
}

struct scn_global_victory *scn_get_global_victory(const struct scn_description *desc)
{
	return (struct scn_global_victory*)((const unsigned char*)&desc[1] + desc->description_length + 1);
}

void Scenario::dump() const {
	char buf[4096];
	if (scn.hdr3->filename_length > sizeof buf) {
		strncpy(buf, scn.hdr3->filename, sizeof buf);
		buf[(sizeof buf) - 1] = '\0';
	} else {
		strncpy(buf, scn.hdr3->filename, scn.hdr3->filename_length);
		buf[scn.hdr3->filename_length] = '\0';
	}
	printf("filename: %s\n", buf);

	const struct scn_description *desc = scn_get_description(&scn);
	if (desc->description_length > sizeof buf) {
		strncpy(buf, desc->description, sizeof buf);
		buf[(sizeof buf) - 1] = '\0';
	} else {
		strncpy(buf, desc->description, desc->description_length);
		buf[desc->description_length] = '\0';
	}
	printf("description: %s\n", buf);

	uint16_t player_count = scn.hdr2->player_count;
	printf("player count: %" PRIu16 "\n", player_count);

	puts("global victory conditions:");
	struct scn_global_victory *global_victory = scn_get_global_victory(desc);
	for (unsigned i = 0; i < player_count; ++i) {
		switch (global_victory->type) {
		case 0:
			printf("%2u: standard\n", i);
			global_victory = (struct scn_global_victory*)((unsigned char*)global_victory + 1);
			continue;
		case 1:
			printf("%2u: conquest\n", i);
			break;
		default:
			printf("%2u: unknown (code %" PRIu8 ")\n", i, global_victory->type);
			break;
		}
		++global_victory;
	}
}
