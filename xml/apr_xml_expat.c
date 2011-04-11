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

#if APU_USE_EXPAT
#include "apr_xml.h"

#if defined(HAVE_XMLPARSE_XMLPARSE_H)
#include <xmlparse/xmlparse.h>
#elif defined(HAVE_XMLTOK_XMLPARSE_H)
#include <xmltok/xmlparse.h>
#elif defined(HAVE_XML_XMLPARSE_H)
#include <xml/xmlparse.h>
#else
#include <expat.h>
#endif

typedef enum XML_Error XML_Error;

#include "apr_xml_internal.h"

static apr_status_t cleanup_parser(void *ctx)
{
    apr_xml_parser *parser = ctx;

    XML_ParserFree(parser->xp);
    parser->xp = NULL;

    return APR_SUCCESS;
}
static apr_status_t do_parse(apr_xml_parser *parser,
                             const char *data, apr_size_t len,
                             int is_final)
{
    if (parser->xp == NULL) {
        parser->error = APR_XML_ERROR_PARSE_DONE;
    }
    else {
        int rv = XML_Parse(parser->xp, data, (int)len, is_final);

        if (rv == 0) {
            parser->error = APR_XML_ERROR_EXPAT;
            parser->xp_err = XML_GetErrorCode(parser->xp);
            parser->xp_msg = XML_ErrorString(parser->xp_err);
        }
    }

    /* ### better error code? */
    return parser->error ? APR_EGENERAL : APR_SUCCESS;
}


static XMLParserImpl xml_parser_expat = {
    do_parse,
    cleanup_parser
};

XMLParserImpl* apr_xml_get_parser_impl(void) { return &xml_parser_expat; }
static const char APR_KW_DAV[] = { 0x44, 0x41, 0x56, 0x3A, '\0' };

#if XML_MAJOR_VERSION > 1
/* Stop the parser if an entity declaration is hit. */
static void entity_declaration(void *userData, const XML_Char *entityName,
                               int is_parameter_entity, const XML_Char *value,
                               int value_length, const XML_Char *base,
                               const XML_Char *systemId, const XML_Char *publicId,
                               const XML_Char *notationName)
{
    apr_xml_parser *parser = userData;

    XML_StopParser(parser->xp, XML_FALSE);
}
#else
/* A noop default_handler. */
static void default_handler(void *userData, const XML_Char *s, int len)
{
}
#endif

apr_xml_parser* apr_xml_parser_create_internal(apr_pool_t *pool,
    void *start_func, void *end_func, void *cdata_func)
{
    apr_xml_parser *parser = apr_pcalloc(pool, sizeof(*parser));

    parser->impl = apr_xml_get_parser_impl();

    parser->p = pool;
    parser->doc = apr_pcalloc(pool, sizeof(*parser->doc));

    parser->doc->namespaces = apr_array_make(pool, 5, sizeof(const char *));

    /* ### is there a way to avoid hard-coding this? */
    apr_xml_insert_uri(parser->doc->namespaces, APR_KW_DAV);

    parser->xp = XML_ParserCreate(NULL);
    if (parser->xp == NULL) {
        (*apr_pool_abort_get(pool))(APR_ENOMEM);
        return NULL;
    }

    apr_pool_cleanup_register(pool, parser, cleanup_parser,
                              apr_pool_cleanup_null);

    XML_SetUserData(parser->xp, parser);
    XML_SetElementHandler(parser->xp, start_func, end_func);
    XML_SetCharacterDataHandler(parser->xp, cdata_func);

    /* Prevent the "billion laughs" attack against expat by disabling
     * internal entity expansion.  With 2.x, forcibly stop the parser
     * if an entity is declared - this is safer and a more obvious
     * failure mode.  With older versions, installing a noop
     * DefaultHandler means that internal entities will be expanded as
     * the empty string, which is also sufficient to prevent the
     * attack. */
#if XML_MAJOR_VERSION > 1
    XML_SetEntityDeclHandler(parser->xp, entity_declaration);
#else
    XML_SetDefaultHandler(parser->xp, default_handler);
#endif

    return parser;
}
#endif
