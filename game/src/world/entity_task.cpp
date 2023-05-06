#include "../game.hpp"

namespace aoe {

EntityTask::EntityTask(IdPoolRef ref, uint32_t x, uint32_t y) : type(EntityTaskType::move), ref1(ref), ref2(invalid_ref), x(x), y(y), info_type(0), info_value(0) {}

EntityTask::EntityTask(EntityTaskType type, IdPoolRef ref1, IdPoolRef ref2) : type(type), ref1(ref1), ref2(ref2), x(0), y(0), info_type(0), info_value(0) {}

EntityTask::EntityTask(IdPoolRef ref, EntityType t) : type(EntityTaskType::train_unit), ref1(ref), ref2(invalid_ref), x(0), y(0), info_type((unsigned)EntityIconType::unit), info_value((unsigned)t) {}

}
