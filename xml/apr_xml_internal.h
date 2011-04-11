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

#ifndef APR_XML_INTERNAL_H
#define APR_XML_INTERNAL_H


struct XMLParserImpl {
    apr_status_t (*Parse)(apr_xml_parser*, const char*, apr_size_t, int);
    apr_status_t (*cleanup)(void*);
};
typedef struct XMLParserImpl XMLParserImpl;
XMLParserImpl* apr_xml_get_parser_impl(void);


/* the real (internal) definition of the parser context */
struct apr_xml_parser {
    apr_xml_doc *doc;           /* the doc we're parsing */
    apr_pool_t *p;              /* the pool we allocate from */
    apr_xml_elem *cur_elem;     /* current element */

    int error;                  /* an error has occurred */
#define APR_XML_ERROR_EXPAT             1
#define APR_XML_ERROR_PARSE_DONE        2
/* also: public APR_XML_NS_ERROR_* values (if any) */

    XML_Parser xp;              /* the actual (Expat) XML parser */
    XML_Error xp_err;      /* stored Expat error code */
    const char *xp_msg;
    XMLParserImpl *impl;
};

apr_xml_parser* apr_xml_parser_create_internal(apr_pool_t*, void*, void*, void*);

#endif
