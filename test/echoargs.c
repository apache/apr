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

/* Simple echo daemon, designed to be used for network throughput
 * benchmarks. The aim is to allow us to monitor changes in performance
 * of APR networking code, nothing more.
 */

#include <stdio.h>

#include <apr.h>
#include <apr_general.h>
#include <apr_file_io.h>

int main(int argc, const char * const *argv, const char* const* env)
{
    apr_pool_t* pool;
    apr_file_t* file;
    int i;

    apr_app_initialize(&argc, &argv, &env);
    apr_pool_create(&pool, NULL);

    apr_file_open_stdout(&file, pool);
    for (i = 1; i < argc; i++)
    {
        if (i > 1) apr_file_puts(",", file);
        apr_file_puts(argv[i], file);
    }

    apr_terminate();

    return 0;
}
