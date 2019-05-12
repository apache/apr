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
#include "apr_general.h"
#include "apr_xml.h"
#include "abts.h"
#include "testutil.h"

static apr_status_t create_dummy_file_error(abts_case *tc, apr_pool_t *p,
                                            apr_file_t **fd)
{
    int i;
    apr_status_t rv;
    apr_off_t off = 0L;
    char template[] = "data/testxmldummyerrorXXXXXX";

    rv = apr_file_mktemp(fd, template, APR_FOPEN_CREATE | APR_FOPEN_TRUNCATE | APR_FOPEN_DELONCLOSE |
                         APR_FOPEN_READ | APR_FOPEN_WRITE | APR_FOPEN_EXCL, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    if (rv != APR_SUCCESS)
        return rv;

    rv = apr_file_puts("<?xml version=\"1.0\" ?>\n<maryx>"
                       "<had a=\"little\"/><lamb/>\n", *fd);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    for (i = 0; i < 5000; i++) {
        rv = apr_file_puts("<hmm roast=\"lamb\" "
                           "for=\"dinner\">yummy</hmm>\n", *fd);
        ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    }

    rv = apr_file_puts("</mary>\n", *fd);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_file_seek(*fd, APR_SET, &off);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    return rv;
}

static apr_status_t create_dummy_file(abts_case *tc, apr_pool_t *p,
                                      apr_file_t **fd)
{
    int i;
    apr_status_t rv;
    apr_off_t off = 0L;
    char template[] = "data/testxmldummyXXXXXX";

    rv = apr_file_mktemp(fd, template, APR_FOPEN_CREATE | APR_FOPEN_TRUNCATE | APR_FOPEN_DELONCLOSE |
                         APR_FOPEN_READ | APR_FOPEN_WRITE | APR_FOPEN_EXCL, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    if (rv != APR_SUCCESS)
        return rv;

    rv = apr_file_puts("<?xml version=\"1.0\" ?>\n<mary>\n", *fd);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    for (i = 0; i < 5000; i++) {
        rv = apr_file_puts("<hmm roast=\"lamb\" "
                           "for=\"dinner &lt;&gt;&#x3D;\">yummy</hmm>\n", *fd);
        ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    }

    rv = apr_file_puts("</mary>\n", *fd);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_file_seek(*fd, APR_SET, &off);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    return rv;
}

static void dump_xml(abts_case *tc, apr_xml_elem *e, int level)
{
    apr_xml_attr *a;
    apr_xml_elem *ec;

    if (level == 0) {
        ABTS_STR_EQUAL(tc, "mary", e->name);
    } else {
        ABTS_STR_EQUAL(tc, "hmm", e->name);
    }

    if (e->attr) {
        a = e->attr;
        ABTS_PTR_NOTNULL(tc, a);
        ABTS_STR_EQUAL(tc, "for", a->name);
        ABTS_STR_EQUAL(tc, "dinner <>=", a->value);
        a = a->next;
        ABTS_PTR_NOTNULL(tc, a);
        ABTS_STR_EQUAL(tc, "roast", a->name);
        ABTS_STR_EQUAL(tc, "lamb", a->value);
    }
    if (e->first_child) {
        ec = e->first_child;
        while (ec) {
            dump_xml(tc, ec, level + 1);
            ec = ec->next;
        }
    }
}

static void test_xml_parser(abts_case *tc, void *data)
{
    apr_file_t *fd;
    apr_xml_parser *parser;
    apr_xml_doc *doc;
    apr_status_t rv;

    rv = create_dummy_file(tc, p, &fd);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    if (rv != APR_SUCCESS)
        return;

    rv = apr_xml_parse_file(p, &parser, &doc, fd, 2000);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    dump_xml(tc, doc->root, 0);

    rv = apr_file_close(fd);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = create_dummy_file_error(tc, p, &fd);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    if (rv != APR_SUCCESS)
        return;

    rv = apr_xml_parse_file(p, &parser, &doc, fd, 2000);
    ABTS_TRUE(tc, rv != APR_SUCCESS);
}

static void test_billion_laughs(abts_case *tc, void *data)
{
    apr_file_t *fd;
    apr_xml_parser *parser;
    apr_xml_doc *doc;
    apr_status_t rv;

    rv = apr_file_open(&fd, "data/billion-laughs.xml", 
                       APR_FOPEN_READ, 0, p);
    APR_ASSERT_SUCCESS(tc, "open billion-laughs.xml", rv);

    rv = apr_xml_parse_file(p, &parser, &doc, fd, 2000);
    ABTS_TRUE(tc, rv != APR_SUCCESS);

    apr_file_close(fd);
}

static void roundtrip(abts_case* tc, char* xml, char* expected, int lineno)
{
    apr_xml_parser *parser;
    apr_xml_doc *doc;
    apr_status_t rv;
    const char *actual;
    apr_size_t len = strlen(xml);
    apr_size_t i;
    apr_pool_t *pool;

    apr_pool_create(&pool, p);

    parser = apr_xml_parser_create(pool);

    /* Feed parser by one character for better test coverage. */
    for (i = 0; i < len; i++) {
        rv = apr_xml_parser_feed(parser, xml + i, 1);
        ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
        if (rv != APR_SUCCESS)
            return;
    }

    rv = apr_xml_parser_done(parser, &doc);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    if (rv != APR_SUCCESS)
        return;

    apr_xml_quote_elem(pool, doc->root);

    apr_xml_to_text(pool, doc->root, APR_XML_X2T_FULL_NS_LANG, doc->namespaces, NULL, &actual, NULL);

    abts_str_equal(tc, expected, actual, lineno);

    apr_pool_destroy(pool);
}

static void test_xml_roundtrip(abts_case *tc, void *data)
{
    roundtrip(tc,
              "<test attr='val'></test>",
              "<test attr=\"val\" xmlns:ns0=\"DAV:\"/>",
              __LINE__);

    roundtrip(tc,
              "<test>aa<q/></test>",
              "<test xmlns:ns0=\"DAV:\">aa<q/></test>",
              __LINE__);

    roundtrip(tc,
              "<?xml version='1.0'?><test>aa<q/>bb</test>",
              "<test xmlns:ns0=\"DAV:\">aa<q/>bb</test>",
              __LINE__);

    roundtrip(tc,
              "<test xmlns:ns1='NS1:'>aa<ns1:q/></test>",
              "<test xmlns:ns1=\"NS1:\" xmlns:ns0=\"DAV:\">aa<ns1:q/></test>",
              __LINE__);

    roundtrip(tc,
              "<test xmlns='default'>aa<q/></test>",
              "<ns1:test xmlns:ns1=\"default\" xmlns:ns0=\"DAV:\">"
              "aa<ns1:q/>"
              "</ns1:test>",
              __LINE__);

    roundtrip(tc,
              "<test xmlns='default' xmlns:ns1='NS1:'>"
              "aa<ns1:q/>"
              "</test>",
              "<ns2:test xmlns:ns2=\"default\" xmlns:ns1=\"NS1:\" xmlns:ns0=\"DAV:\">"
              "aa<ns1:q/>"
              "</ns2:test>",
              __LINE__);

    roundtrip(tc,
              "<test>"
              "&lt;sender&gt;John Smith&lt;/sender&gt;"
              "</test>",
              "<test xmlns:ns0=\"DAV:\">"
              "&lt;sender&gt;John Smith&lt;/sender&gt;"
              "</test>",
              __LINE__);

    roundtrip(tc,
              "<test>"
              "<![CDATA[<sender>John Smith</sender>]]>"
              "</test>",
              "<test xmlns:ns0=\"DAV:\">"
              "&lt;sender&gt;John Smith&lt;/sender&gt;"
              "</test>",
              __LINE__);

    roundtrip(tc,
              "<elem>   abc    def    </elem>",
              "<elem xmlns:ns0=\"DAV:\">   abc    def    </elem>",
              __LINE__);

    roundtrip(tc,
              "<elem>   </elem>",
              "<elem xmlns:ns0=\"DAV:\">   </elem>",
              __LINE__);

    roundtrip(tc,
              "<?xml version='1.0' ?>\n"
              "<elem>   </elem>",
              "<elem xmlns:ns0=\"DAV:\">   </elem>",
              __LINE__);
}

static void get_xml_error(abts_case* tc,
                          char *errbuf,
                          apr_size_t errbufsize,
                          const char* xml)
{
    apr_xml_parser *parser;
    apr_xml_doc *doc;
    apr_status_t rv;
    apr_size_t len = strlen(xml);
    apr_pool_t *pool;

    strcpy(errbuf, "");

    apr_pool_create(&pool, p);

    parser = apr_xml_parser_create(pool);

    rv = apr_xml_parser_feed(parser, xml, len);

    if (rv == APR_SUCCESS) {
        rv = apr_xml_parser_done(parser, &doc);
        ABTS_INT_EQUAL(tc, APR_EGENERAL, rv);
    }

    if (rv != APR_SUCCESS) {
        apr_xml_parser_geterror(parser, errbuf, errbufsize);
    }

    apr_pool_destroy(pool);
}

static void test_xml_parser_geterror(abts_case *tc, void *data)
{
    char errbuf[256];

    get_xml_error(tc, errbuf, sizeof(errbuf),
                  "<elem");

#if APU_USE_EXPAT
    ABTS_STR_EQUAL(tc, "XML parser error code: unclosed token (5)", errbuf);
#endif

    get_xml_error(tc, errbuf, sizeof(errbuf),
                  "<elem1><elem2></elem1>");

#if APU_USE_EXPAT
    ABTS_STR_EQUAL(tc, "XML parser error code: mismatched tag (7)", errbuf);
#endif
}

abts_suite *testxml(abts_suite *suite)
{
    suite = ADD_SUITE(suite);

    abts_run_test(suite, test_xml_parser, NULL);
    abts_run_test(suite, test_billion_laughs, NULL);
    abts_run_test(suite, test_xml_roundtrip, NULL);
    abts_run_test(suite, test_xml_parser_geterror, NULL);

    return suite;
}
