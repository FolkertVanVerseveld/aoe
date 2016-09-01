#include "comm.h"
#include <assert.h>
#include "dbg.h"
#include "todo.h"

int commhnd423D10(struct comm *this, unsigned int id)
{
	stub
	assert(id < 15);
	return id < 1 || id > this->player_id_max ? 0 : this->tbl16D4[id];
}
