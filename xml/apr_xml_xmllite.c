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
#include "apr_arch_utf8.h"
#include "apr_xml.h"

typedef struct xmllite_parser_s* XML_Parser;
typedef int XML_Error;

#include "apr_xml_internal.h"

#define CINTERFACE
#define COBJMACROS
#define interface struct
typedef void * LPMSG;

#include <xmllite.h>

#include "apr_xml_internal.h"

typedef struct xml_stream_t {
    ISequentialStream sequental_stream;
    ULONG refcount;
    const char *data;
    apr_size_t remaining;
    int is_final;
} xml_stream_t;

struct xmllite_parser_s
{
    IXmlReader *xml_reader;
    xml_stream_t *input_stream;
    apr_pool_t *iterpool;
    apr_status_t (*current_state)(apr_xml_parser *parser, apr_pool_t *scratch_pool);

    void (*start_func)(void *userdata, const char *name, const char **attrs);
    void (*end_func)(void *userdata, const char *name);
    void (*cdata_func)(void *userdata, const char *data, int len);
};

static HRESULT STDMETHODCALLTYPE
stream_QueryInterface(ISequentialStream * This, REFIID riid, void **ppvObject)
{
    xml_stream_t *obj = CONTAINING_RECORD(This, xml_stream_t, sequental_stream);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_ISequentialStream))
    {
        InterlockedIncrement(&obj->refcount);
        *ppvObject = &obj->sequental_stream;
        return S_OK;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG STDMETHODCALLTYPE
stream_AddRef(ISequentialStream * This)
{
    xml_stream_t *obj = CONTAINING_RECORD(This, xml_stream_t, sequental_stream);
    return InterlockedIncrement(&obj->refcount);
}

static ULONG STDMETHODCALLTYPE
stream_Release(ISequentialStream * This)
{
    xml_stream_t *obj = CONTAINING_RECORD(This, xml_stream_t, sequental_stream);
    ULONG refcount = InterlockedDecrement(&obj->refcount);

    if (refcount == 0) {
        free(obj);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE
stream_Read(ISequentialStream * This,
            void *pv,
            ULONG cb,
            ULONG *pcbRead)
{
    xml_stream_t *obj = CONTAINING_RECORD(This, xml_stream_t, sequental_stream);
    ULONG read = cb;

    if (read > obj->remaining) {
        read = (ULONG) obj->remaining;
    }

    memcpy(pv, obj->data, read);
    obj->data += read;
    obj->remaining -= read;
    *pcbRead = read;

    if (read == cb) {
        return S_OK;
    }
    else if (read < cb && !obj->is_final) {
        return E_PENDING;
    }
    else
    {
        return S_FALSE;
    }
}

static HRESULT STDMETHODCALLTYPE
stream_Write(ISequentialStream * This,
             const void *pv,
             ULONG cb,
             ULONG *pcbWritten)
{
    xml_stream_t *obj = CONTAINING_RECORD(This, xml_stream_t, sequental_stream);

    return E_NOTIMPL;
}

static ISequentialStreamVtbl stream_vtable =
{
    stream_QueryInterface,
    stream_AddRef,
    stream_Release,
    stream_Read,
    stream_Write
};

static apr_status_t cleanup_xml_stream(void *ctx)
{
    xml_stream_t *xml_stream = ctx;

    ISequentialStream_Release(&xml_stream->sequental_stream);

    return APR_SUCCESS;
}

static xml_stream_t *create_xml_stream(apr_pool_t *pool)
{
    xml_stream_t *xml_stream = malloc(sizeof(*xml_stream));
    if (xml_stream == NULL) {
        return NULL;
    }

    memset(xml_stream, 0, sizeof(*xml_stream));
    xml_stream->sequental_stream.lpVtbl = &stream_vtable;
    xml_stream->refcount = 1;

    apr_pool_cleanup_register(pool, xml_stream, cleanup_xml_stream,
                              apr_pool_cleanup_null);

    return xml_stream;
}

static apr_status_t cleanup_parser(void *ctx)
{
    apr_xml_parser *parser = ctx;

    if (parser->xp->xml_reader) {
        IXmlReader_Release(parser->xp->xml_reader);
        parser->xp->xml_reader = NULL;
    }

    return APR_SUCCESS;
}

static apr_status_t wstr2utf(const char **utf_p, apr_size_t *utf_len_p,
                             LPCWSTR wstr, apr_size_t wlen,
                             apr_pool_t *pool)
{
    apr_size_t result_len;
    char *result;

    if (wlen > 0) {
        apr_status_t status;
        apr_size_t bufsize = wlen * 3;
        apr_size_t outbytes = bufsize;

        result = apr_palloc(pool, outbytes + 1);
        if (!result) {
            return APR_ENOMEM;
        }

        status = apr_conv_ucs2_to_utf8(wstr, &wlen, result, &outbytes);
        if (status) {
            return status;
        }

        result_len = bufsize - outbytes;
        result[result_len] = 0;
    }
    else {
        result = "";
        result_len = 0;
    }

    *utf_p = result;
    if (utf_len_p) {
        *utf_len_p = result_len;
    }

    return APR_SUCCESS;
}

static char * get_xmllite_errmsg(HRESULT hr)
{
    switch(hr)
    {
        case MX_E_INPUTEND:
            return "unexpected end of input";
        case MX_E_ENCODING:
            return "unrecognized encoding";
        case MX_E_ENCODINGSWITCH:
            return "unable to switch the encoding";
        case MX_E_ENCODINGSIGNATURE:
            return "unrecognized input signature";
        case WC_E_WHITESPACE:
            return "whitespace expected";
        case WC_E_SEMICOLON:
            return "semicolon expected";
        case WC_E_GREATERTHAN:
            return "'>' expected";
        case WC_E_QUOTE:
            return "quote expected";
        case WC_E_EQUAL:
            return "equal expected";
        case WC_E_LESSTHAN:
            return "well-formedness constraint: no '<' in attribute value";
        case WC_E_HEXDIGIT:
            return "hexadecimal digit expected";
        case WC_E_DIGIT:
            return "decimal digit expected";
        case WC_E_LEFTBRACKET:
            return "'[' expected";
        case WC_E_LEFTPAREN:
            return "'(' expected";
        case WC_E_XMLCHARACTER:
            return "illegal xml character";
        case WC_E_NAMECHARACTER:
            return "illegal name character";
        case WC_E_SYNTAX:
            return "incorrect document syntax";
        case WC_E_CDSECT:
            return "incorrect CDATA section syntax";
        case WC_E_COMMENT:
            return "incorrect comment syntax";
        case WC_E_CONDSECT:
            return "incorrect conditional section syntax";
        case WC_E_DECLATTLIST:
            return "incorrect ATTLIST declaration syntax";
        case WC_E_DECLDOCTYPE:
            return "incorrect DOCTYPE declaration syntax";
        case WC_E_DECLELEMENT:
            return "incorrect ELEMENT declaration syntax";
        case WC_E_DECLENTITY:
            return "incorrect ENTITY declaration syntax";
        case WC_E_DECLNOTATION:
            return "incorrect NOTATION declaration syntax";
        case WC_E_NDATA:
            return "NDATA expected";
        case WC_E_PUBLIC:
            return "PUBLIC expected";
        case WC_E_SYSTEM:
            return "SYSTEM expected";
        case WC_E_NAME:
            return "name expected";
        case WC_E_ROOTELEMENT:
            return "one root element";
        case WC_E_ELEMENTMATCH:
            return "well-formedness constraint: element type match";
        case WC_E_UNIQUEATTRIBUTE:
            return "well-formedness constraint: unique attribute spec";
        case WC_E_TEXTXMLDECL:
            return "text/xmldecl not at the beginning of input";
        case WC_E_LEADINGXML:
            return "leading \"xml\"";
        case WC_E_TEXTDECL:
            return "incorrect text declaration syntax";
        case WC_E_XMLDECL:
            return "incorrect xml declaration syntax";
        case WC_E_ENCNAME:
            return "incorrect encoding name syntax";
        case WC_E_PUBLICID:
            return "incorrect public identifier syntax";
        case WC_E_PESINTERNALSUBSET:
            return "well-formedness constraint: pes in internal subset";
        case WC_E_PESBETWEENDECLS:
            return "well-formedness constraint: pes between declarations";
        case WC_E_NORECURSION:
            return "well-formedness constraint: no recursion";
        case WC_E_ENTITYCONTENT:
            return "entity content not well formed";
        case WC_E_UNDECLAREDENTITY:
            return "well-formedness constraint: undeclared entity";
        case WC_E_PARSEDENTITY:
            return "well-formedness constraint: parsed entity";
        case WC_E_NOEXTERNALENTITYREF:
            return "well-formedness constraint: no external entity references";
        case WC_E_PI:
            return "incorrect processing instruction syntax";
        case WC_E_SYSTEMID:
            return "incorrect system identifier syntax";
        case WC_E_QUESTIONMARK:
            return "'?' expected";
        case WC_E_CDSECTEND:
            return "no ']]>' in element content";
        case WC_E_MOREDATA:
            return "not all chunks of value have been read";
        case WC_E_DTDPROHIBITED:
            return "DTD was found but is prohibited";
        case WC_E_INVALIDXMLSPACE:
            return "xml:space attribute with invalid value";
        case NC_E_QNAMECHARACTER:
            return "illegal qualified name character";
        case NC_E_QNAMECOLON:
            return "multiple colons in qualified name";
        case NC_E_NAMECOLON:
            return "colon in name";
        case NC_E_DECLAREDPREFIX:
            return "declared prefix";
        case NC_E_UNDECLAREDPREFIX:
            return "undeclared prefix";
        case NC_E_EMPTYURI:
            return "non default namespace with empty uri";
        case NC_E_XMLPREFIXRESERVED:
            return "\"xml\" prefix is reserved and must have the "
                   "http://www.w3.org/XML/1998/namespace URI";
        case NC_E_XMLNSPREFIXRESERVED:
            return "\"xmlns\" prefix is reserved for use by XML";
        case NC_E_XMLURIRESERVED:
            return "xml namespace URI (http://www.w3.org/XML/1998/namespace) must "
                   "be assigned only to prefix \"xml\"";
        case NC_E_XMLNSURIRESERVED:
            return "xmlns namespace URI (http://www.w3.org/2000/xmlns/) is "
                   "reserved and must not be used";
        case SC_E_MAXELEMENTDEPTH:
            return "element depth exceeds limit";
        case SC_E_MAXENTITYEXPANSION:
            return "entity expansion exceeds limit";
        case XML_E_INVALID_DECIMAL:
            return "character in character entity is not a decimal digit "
                   "as was expected.";
        case XML_E_INVALID_HEXIDECIMAL:
            return "character in character entity is not a hexadecimal "
                   "digit as was expected.";
        case XML_E_INVALID_UNICODE:
            return "character entity has invalid Unicode value.";
        default:
            return "";
    }
}

static apr_status_t handle_xmllite_err(apr_xml_parser *parser, HRESULT hr)
{
    parser->xp_err = hr;

    parser->xp_msg = get_xmllite_errmsg(hr);
    /* this misnomer is used as a test for (any) parser error. */
    parser->error = APR_XML_ERROR_EXPAT;

    return APR_EGENERAL;
}

static apr_status_t
cdata_state(apr_xml_parser *parser,
            apr_pool_t *scratch_pool);

static apr_status_t
read_state(apr_xml_parser *parser,
           apr_pool_t *scratch_pool)
{
    HRESULT hr;
    XmlNodeType node_type;
    apr_status_t status;

    hr = IXmlReader_Read(parser->xp->xml_reader, &node_type);
    if (hr == E_PENDING) {
        return APR_EAGAIN;
    }
    else if (FAILED(hr)) {
        return handle_xmllite_err(parser, hr);
    }

    if (node_type == XmlNodeType_Element) {
        LPCWSTR wname;
        UINT wname_len;
        ULONG attr_count;
        char **attrs;
        const char* elem_name;

        hr = IXmlReader_GetQualifiedName(parser->xp->xml_reader,
                                         &wname, &wname_len);
        if (FAILED(hr)) {
            return handle_xmllite_err(parser, hr);
        }

        status = wstr2utf(&elem_name, NULL, wname, wname_len, scratch_pool);
        if (status) {
            return status;
        }

        hr = IXmlReader_GetAttributeCount(parser->xp->xml_reader, &attr_count);
        if (FAILED(hr)) {
            return handle_xmllite_err(parser, hr);
        }

        if (attr_count > 0) {
            ULONG i;

            attrs = apr_palloc(parser->p, sizeof(char*) * (attr_count + 1) * 2);

            hr = IXmlReader_MoveToFirstAttribute(parser->xp->xml_reader);
            if (FAILED(hr)) {
                return handle_xmllite_err(parser, hr);
            }

            for (i = 0; i < attr_count; i++) {
                hr = IXmlReader_GetQualifiedName(parser->xp->xml_reader,
                                                 &wname, &wname_len);
                if (FAILED(hr)) {
                    return handle_xmllite_err(parser, hr);
                }

                status = wstr2utf(&attrs[i * 2], NULL, wname, wname_len,
                                  scratch_pool);
                if (status) {
                    return status;
                }

                hr = IXmlReader_GetValue(parser->xp->xml_reader,
                                         &wname, &wname_len);
                if (FAILED(hr)) {
                    return handle_xmllite_err(parser, hr);
                }

                status = wstr2utf(&attrs[i * 2 + 1], NULL, wname, wname_len,
                                  scratch_pool);
                if (status) {
                    return status;
                }

                hr = IXmlReader_MoveToNextAttribute(parser->xp->xml_reader);
                if (FAILED(hr)) {
                    return handle_xmllite_err(parser, hr);
                }
            }

            attrs[i * 2] = NULL;
            attrs[i * 2 + 1] = NULL;

            hr = IXmlReader_MoveToElement(parser->xp->xml_reader);
            if (FAILED(hr)) {
                return handle_xmllite_err(parser, hr);
            }
        }
        else {
            static char* no_attrs[] = { NULL, NULL };
            attrs = no_attrs;
        }

        parser->xp->start_func(parser, elem_name, attrs);

        if (IXmlReader_IsEmptyElement(parser->xp->xml_reader)) {
            parser->xp->end_func(parser, elem_name);
        }
    }
    else if (node_type == XmlNodeType_EndElement) {
        LPCWSTR wname;
        UINT wname_len;
        const char *elem_name;

        hr = IXmlReader_GetQualifiedName(parser->xp->xml_reader,
                                         &wname, &wname_len);
        if (FAILED(hr)) {
            return handle_xmllite_err(parser, hr);
        }

        status = wstr2utf(&elem_name, NULL, wname, wname_len, scratch_pool);
        if (status) {
            return status;
        }

        parser->xp->end_func(parser, elem_name);
    }
    else if (node_type == XmlNodeType_CDATA ||
             node_type == XmlNodeType_Text) {
        parser->xp->current_state = cdata_state;
    }
    else if (node_type == XmlNodeType_Whitespace) {
        UINT depth;
        hr = IXmlReader_GetDepth(parser->xp->xml_reader, &depth);
        if (FAILED(hr)) {
            return handle_xmllite_err(parser, hr);
        }

        /* Report whitespaces as cdata (the same as Expat does), but
           ignore them when depth == 0. */
        if (depth > 0) {
            parser->xp->current_state = cdata_state;
        }
    }
    else if (node_type == XmlNodeType_None)
    {
        return APR_EOF;
    }

    return APR_SUCCESS;
}

static apr_status_t
cdata_state(apr_xml_parser *parser,
            apr_pool_t *scratch_pool)
{
    HRESULT hr;
    apr_status_t status;
    WCHAR buf[4000];
    UINT read_count;

    hr = IXmlReader_ReadValueChunk(parser->xp->xml_reader, buf,
                                   sizeof(buf) / sizeof(buf[0]),
                                   &read_count);
    if (hr == E_PENDING) {
        return APR_EAGAIN;
    }
    else if (FAILED(hr)) {
        return handle_xmllite_err(parser, hr);
    }

    if (read_count > 0) {
        const char *cdata;
        apr_size_t cdata_len;

        status = wstr2utf(&cdata, &cdata_len, buf, read_count, scratch_pool);
        if (status) {
            return status;
        }

        parser->xp->cdata_func(parser, cdata, (int) cdata_len);
    }

    if (hr == S_FALSE) {
        parser->xp->current_state = read_state;
    }

    return APR_SUCCESS;
}


static apr_status_t do_parse(apr_xml_parser *parser,
                             const char *data, apr_size_t len,
                             int is_final)
{
    apr_status_t status;
    apr_pool_t *iterpool = parser->xp->iterpool;

    parser->xp->input_stream->data = data;
    parser->xp->input_stream->remaining = len;
    parser->xp->input_stream->is_final = is_final;

    while (TRUE) {
        apr_pool_clear(iterpool);

        status = parser->xp->current_state(parser, iterpool);
        if (status != APR_SUCCESS) {
            break;
        }
    }

    if (status == APR_EAGAIN || status == APR_EOF) {
        status = APR_SUCCESS;
    }

    return status;
}

static XMLParserImpl xml_parser_xmllite = {
    do_parse,
    cleanup_parser
};

XMLParserImpl* apr_xml_get_parser_impl(void) { return &xml_parser_xmllite; }
static const char APR_KW_DAV[] = { 0x44, 0x41, 0x56, 0x3A, '\0' };

apr_xml_parser* apr_xml_parser_create_internal(apr_pool_t *pool,
                                               void *start_func,
                                               void *end_func,
                                               void *cdata_func)
{
    apr_xml_parser *parser = apr_pcalloc(pool, sizeof(*parser));
    HRESULT hr;

    parser->impl = apr_xml_get_parser_impl();
    parser->p = pool;
    parser->doc = apr_pcalloc(pool, sizeof(*parser->doc));
    parser->doc->namespaces = apr_array_make(pool, 5, sizeof(const char *));

    /* ### is there a way to avoid hard-coding this? */
    apr_xml_insert_uri(parser->doc->namespaces, APR_KW_DAV);
    parser->xp = apr_pcalloc(pool, sizeof(struct xmllite_parser_s));
    parser->xp->current_state = read_state;
    parser->xp->start_func = start_func;
    parser->xp->end_func = end_func;
    parser->xp->cdata_func = cdata_func;
    apr_pool_create(&parser->xp->iterpool, pool);

    parser->xp->input_stream = create_xml_stream(pool);
    if (parser->xp->input_stream == NULL) {
        (*apr_pool_abort_get(pool))(APR_ENOMEM);
        return NULL;
    }

    hr = CreateXmlReader(&IID_IXmlReader, &parser->xp->xml_reader, NULL);
    if (FAILED(hr)) {
        return NULL;
    }
    apr_pool_cleanup_register(pool, parser, cleanup_parser, apr_pool_cleanup_null);

    hr = IXmlReader_SetInput(parser->xp->xml_reader,
                             (IUnknown*) &parser->xp->input_stream->sequental_stream);
    if (FAILED(hr)) {
        return NULL;
    }

    return parser;
}
#endif
