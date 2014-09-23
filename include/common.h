 /*
 * common.h
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>

#define ERROR(msg, ...) fprintf(stderr, "%s()  "msg" \n",  __func__, ##__VA_ARGS__)
#define ERRNO(msg, ...) fprintf(stderr, "%s()  "msg": %m \n", __func__, ##__VA_ARGS__)
#define INFO(msg, ...)  fprintf(stderr, "%s()  "msg" \n", __func__, ##__VA_ARGS__)
#define DEBUG(msg, ...) fprintf(stderr, "%s():%d  "msg" \n", __func__, __LINE__, ##__VA_ARGS__)

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

static inline void _cleanup_fn_free_(void *p) {
	free(*(void **)p);
}

#define _cleanup_(fn)   __attribute__((cleanup(fn)))
#define _cleanup_free_  _cleanup_(_cleanup_fn_free_)

#define container_of(ptr, type, field) ({				\
			const typeof(((type *)0)->field) *member = (ptr); \
			(type *)( (char *)member - offsetof(type, field) ); \
		})

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*a))

#endif /* COMMON_H */
