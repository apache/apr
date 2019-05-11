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

#include "apr.h"

#if APU_USE_XMLLITE
#include "apr_xml.h"

typedef void * XML_Parser;
typedef int XML_Error;

#include "apr_xml_internal.h"

#define CINTERFACE
#define COBJMACROS
#define interface struct
typedef void * LPMSG;

#include <xmllite.h>

static apr_status_t cleanup_parser(void *ctx)
{
    apr_xml_parser *parser = ctx;

    return APR_SUCCESS;
}

static apr_status_t xmllite_parse(apr_xml_parser* parser, const char* data,
                                  apr_size_t sz, int final)
{
    return APR_SUCCESS;
}

static XMLParserImpl xml_parser_xmllite = {
    xmllite_parse,
    cleanup_parser
};

static const char APR_KW_DAV[] = { 0x44, 0x41, 0x56, 0x3A, '\0' };

XMLParserImpl* apr_xml_get_parser_impl(void)
{
    return &xml_parser_xmllite;
}


apr_xml_parser* apr_xml_parser_create_internal(apr_pool_t *pool,
                                               void *start_func,
                                               void *end_func,
                                               void *cdata_func)
{
    apr_xml_parser *parser = apr_pcalloc(pool, sizeof(*parser));
    IXmlReader *xml_reader;
    HRESULT hr;

    parser->impl = apr_xml_get_parser_impl();
    parser->p = pool;
    parser->doc = apr_pcalloc(pool, sizeof(*parser->doc));
    parser->doc->namespaces = apr_array_make(pool, 5, sizeof(const char *));

    /* ### is there a way to avoid hard-coding this? */
    apr_xml_insert_uri(parser->doc->namespaces, APR_KW_DAV);
    apr_pool_cleanup_register(pool, parser, cleanup_parser,
                              apr_pool_cleanup_null);

    hr = CreateXmlReader(&IID_IXmlReader, &xml_reader, NULL);
    if (FAILED(hr)) {
        return NULL;
    }

    IXmlReader_Release(xml_reader);

    return parser;
}
#endif
