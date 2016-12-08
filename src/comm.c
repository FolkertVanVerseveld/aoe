#include "comm.h"
#include <assert.h>
#include <string.h>
#include "../dbg.h"
#include "todo.h"
#include "memmap.h"

struct comm *comm_580DA8 = NULL;

int commhnd423D10(struct comm *this, unsigned int id)
{
	stub
	assert(id < 15);
	return id < 1 || id > this->player_id_max ? 0 : this->tbl16D4[id];
}

int comm_no_msg_slot(struct comm *this)
{
	stub
	(void)this;
	halt();
	return 0;
}

static struct game_settings *comm_free_game_settings(struct comm *this)
{
	struct game_settings *opt = this->opt;
	if (opt) {
		this->opt_size = 0;
		delete(opt);
		this->opt = NULL;
	}
	return opt;
}

int comm_opt_grow(struct comm *this, struct game_settings *opt, unsigned size)
{
	if (size > 2000) {
		fprintf(stderr, "%s: bad size = %u\n", __func__, size);
		return 0;
	}
	comm_free_game_settings(this);
	if (opt && size) {
		memcpy(this->opt, opt, size);
		this->opt = opt;
		this->opt_size = size;
	}
	if (this->dword1C)
		this->byte1550 = 1;
	dbgf("Host Set Options to size %d\n", size);
	return 1;
}

struct game_settings *comm_get_settings(struct comm *this, unsigned *opt_size)
{
	*opt_size = this->opt_size;
	return this->opt;
}
