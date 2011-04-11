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

#if APU_USE_LIBXML2
#include "apr_xml.h"

#include <libxml/parser.h>
#include <libxml/xmlerror.h>

typedef xmlParserCtxtPtr XML_Parser;
typedef xmlParserErrors XML_Error;

#include "apr_xml_internal.h"

static apr_status_t cleanup_parser(void *ctx)
{
    apr_xml_parser *parser = ctx;

    xmlFreeParserCtxt(parser->xp);
    parser->xp = NULL;

    return APR_SUCCESS;
}
static int libxml2_parse(apr_xml_parser* parser, const char* data,
                         apr_size_t sz, int final)
{
    parser->xp_err = xmlParseChunk(parser->xp, data, sz, final);
    if (parser->xp_err != 0) {
        xmlErrorPtr errptr = xmlCtxtGetLastError(parser->xp);
        parser->xp_msg = errptr->message;
        /* this misnomer is used as a test for (any) parser error. */
        parser->error = APR_XML_ERROR_EXPAT;
    }
    return parser->xp_err;
}
static XMLParserImpl xml_parser_libxml2 = {
    libxml2_parse,
    cleanup_parser
};

static const char APR_KW_DAV[] = { 0x44, 0x41, 0x56, 0x3A, '\0' };

XMLParserImpl* apr_xml_get_parser_impl(void)
{
    return &xml_parser_libxml2;
}


apr_xml_parser* apr_xml_parser_create_internal(apr_pool_t *pool,
    void *start_func, void *end_func, void *cdata_func)
{
    apr_xml_parser *parser = apr_pcalloc(pool, sizeof(*parser));
    /* FIXME: This is a mismatch.  We should create a single global
     * sax instance and re-use it for every parser.  That means we
     * need an up-front initialisation function.
     */
    xmlSAXHandlerPtr sax = apr_pcalloc(pool, sizeof(xmlSAXHandler));
    sax->startElement = start_func;
    sax->endElement = end_func;
    sax->characters = cdata_func;
    sax->initialized = 1;

    parser->impl = apr_xml_get_parser_impl();

    parser->p = pool;
    parser->doc = apr_pcalloc(pool, sizeof(*parser->doc));

    parser->doc->namespaces = apr_array_make(pool, 5, sizeof(const char *));

    /* ### is there a way to avoid hard-coding this? */
    apr_xml_insert_uri(parser->doc->namespaces, APR_KW_DAV);

    parser->xp = xmlCreatePushParserCtxt(sax, parser, NULL, 0, NULL);
    if (parser->xp == NULL) {
        (*apr_pool_abort_get(pool))(APR_ENOMEM);
        return NULL;
    }

    apr_pool_cleanup_register(pool, parser, cleanup_parser,
                              apr_pool_cleanup_null);

    return parser;
}
#endif
