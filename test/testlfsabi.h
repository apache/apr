/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "abts.h"

#define IDX_apr_dev_t       0
#define IDX_apr_ino_t       1
#define IDX_apr_off_t       2
#define IDX_apr_socklen_t   3
#define IDX_apr_size_t      4
#define IDX_MAX             5

#define GETSIZE(res, type)  res[(IDX_##type)] = sizeof(type)
#define CHECKSIZE(tc, res1, res2, type)                         \
  ABTS_INT_EQUAL(tc, res1[(IDX_##type)], res2[(IDX_##type)]);

extern void get_type_sizes32(int *res);
extern void get_type_sizes64(int *res);

