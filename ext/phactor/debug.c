/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_phactor_debug.h"
#include "php_phactor.h"

void debug_tasks(task_queue_t tasks)
{
    task_t *task = tasks.task;
    int task_count = 0;

    printf("Debugging tasks:\n");

    while (task != NULL) {
        ++task_count;

        printf("%d) Task: %p (%s)", task_count, task, task->task_type == 1 ? "PMT" : "SMT");

        if (task->task_type == 1) {
            printf(", Actor: %p", task->task.pmt.actor);
        } else {
            printf(", Actor: %p, Message zval: %p", task->task.pmt.actor, task->task.smt.message->message);
        }

        printf("\n");

        task = task->next_task;
    }

    printf("\n");
}

void debug_actor_system(struct _actor_system actor_system)
{
    actor_t *current_actor = actor_system.actors;
    int actor_count = 0;

    printf("Debugging actors:\n");

    while (current_actor != NULL) {
        // printf("%d) ref: %32s, actor: %p, actor_obj: %p\n", ++actor_count, current_actor->actor_ref->val, current_actor, current_actor->actor);
        printf("%d) actor: %p, object ref: %u\n", ++actor_count, current_actor, current_actor->actor.handle);
        current_actor = current_actor->next;
    }

    printf("\n");
}
