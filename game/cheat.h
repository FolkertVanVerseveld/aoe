#ifndef AOE_CHEAT_H
#define AOE_CHEAT_H

struct cheat_action {
	char arg0;
	char arg1;
	short player_id;
	short cheat_id;
	char gap6[2];
	void *other;
	unsigned numC;
};

struct cheat_action *create_cheat(struct cheat_action *this, short player_id, short cheat_id);

#endif
