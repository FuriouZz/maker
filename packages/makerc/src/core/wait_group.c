#include "maker_internal.h"

MakerStatus maker_wait_group_init(MakerWaitGroup* group)
{
    MAKER_CHECK(group);

    if (maker_mutex_init(&group->lock) != MAKER_STATUS_OK) {
        goto fail;
    }
    if (maker_cond_init(&group->signal) != MAKER_STATUS_OK) {
        goto cleanup_lock;
    }

    group->counter = 0;

    return MAKER_STATUS_OK;

cleanup_lock:
    maker_mutex_uninit(&group->lock);

fail:
    return MAKER_STATUS_ERROR;
}

MakerStatus maker_wait_group_uninit(MakerWaitGroup* group)
{
    MAKER_CHECK(group);

    maker_mutex_uninit(&group->lock);
    maker_cond_uninit(&group->signal);

    return MAKER_STATUS_OK;
}

void maker_wait_group_add(MakerWaitGroup* group, u32 value)
{
    if (value == 0) return;

    MAKER_ASSERT(group);

    maker_mutex_lock(&group->lock);
    u32 val = MAKER_ATOMIC_ADD(&group->counter, value);

    if (val < 0) {
        MAKER_PANIC("MakerWaitGroup has a negative counter.");
    } else if (val == 0) {
        maker_cond_broadcast(&group->signal);
        if (MAKER_ATOMIC_LOAD(&group->counter) != 0) {
            MAKER_PANIC("Invalid use of MakerWaitGroup.");
        }
    }

    maker_mutex_unlock(&group->lock);
}

void maker_wait_group_done(MakerWaitGroup* group)
{
    maker_wait_group_add(group, -1);
}

void maker_wait_group_wait(MakerWaitGroup* group)
{
    MAKER_ASSERT(group);
    maker_mutex_lock(&group->lock);
    while (MAKER_ATOMIC_LOAD(&group->counter) != 0) {
        maker_cond_wait(&group->signal, &group->lock);
    }
    maker_mutex_unlock(&group->lock);
}
