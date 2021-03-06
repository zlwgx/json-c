#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../src/json.h"

static int test_count = 0;
static int test_pass = 0;

#define ASSERT_EQ_BASE(equality, expect, actual, format) \
    do { \
        test_count++; \
        if (equality) \
            test_pass++; \
        else \
            fprintf(stderr, "%s:%d:\t" #expect " != (" #actual " == " format ")\n", __FILE__, __LINE__, actual); \
    } while (0)

#define ASSERT_EQ_INT(expect, actual) ASSERT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define ASSERT_EQ_DOUBLE(expect, actual) ASSERT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define ASSERT_EQ_SIZE_T(expect, actual) ASSERT_EQ_BASE((expect) == (actual), expect, actual, "%zu")
/* Must not use strcmp to compare strings, because '\0' is a valid UNICODE character */
#define ASSERT_EQ_STRING(expect, actual, length) \
    ASSERT_EQ_BASE(sizeof(expect) - 1 == length && !memcmp(expect, actual, length), expect, actual, "%s")
#define ASSERT_EQ_POINTER(expect, actual) ASSERT_EQ_BASE((expect) == (actual), expect, actual, "%p")

#define TEST_PARSE_RESULT(result, type, json) \
    do { \
        json_value v; \
        json_init(&v); \
        ASSERT_EQ_INT(result, json_parse(&v, json)); \
        ASSERT_EQ_INT(type, json_get_type(&v)); \
        json_free(&v); \
    } while (0)

#define TEST_PARSE_ERROR(json) TEST_PARSE_RESULT(JSON_PARSE_ERROR, JSON_NULL, json)

#define TEST_PARSE_NUMBER(expect, json) \
    do { \
        json_value v; \
        json_init(&v); \
        ASSERT_EQ_INT(JSON_PARSE_OK, json_parse(&v, json)); \
        ASSERT_EQ_INT(JSON_NUMBER, json_get_type(&v)); \
        ASSERT_EQ_DOUBLE(expect, json_get_number(&v)); \
        json_free(&v); \
    } while (0)

#define TEST_PARSE_STRING(expect, json) \
    do { \
        json_value v; \
        json_init(&v); \
        ASSERT_EQ_INT(JSON_PARSE_OK, json_parse(&v, json)); \
        ASSERT_EQ_INT(JSON_STRING, json_get_type(&v)); \
        ASSERT_EQ_STRING(expect, json_get_string(&v), json_get_string_length(&v)); \
        json_free(&v); \
    } while (0)

static void test_parse_true(void)
{
    TEST_PARSE_RESULT(JSON_PARSE_OK, JSON_TRUE, "true");
}

static void test_parse_false(void)
{
    TEST_PARSE_RESULT(JSON_PARSE_OK, JSON_FALSE, "false");
}

static void test_parse_null(void)
{
    TEST_PARSE_RESULT(JSON_PARSE_OK, JSON_NULL, "null");
}

static void test_parse_number(void)
{
    TEST_PARSE_NUMBER(0.0, "0");
    TEST_PARSE_NUMBER(0.0, "0.0");
    TEST_PARSE_NUMBER(0.0, "-0.0");
    TEST_PARSE_NUMBER(123.0, "123");
    TEST_PARSE_NUMBER(3.1415926, "3.1415926");
    TEST_PARSE_NUMBER(1e10, "1e10");
    TEST_PARSE_NUMBER(1E10, "1E10");
    TEST_PARSE_NUMBER(1E+10, "1E+10");
    TEST_PARSE_NUMBER(1E-10, "1E-10");
    TEST_PARSE_NUMBER(-1E+10, "-1E+10");
    TEST_PARSE_NUMBER(-1E-10, "-1E-10");
    TEST_PARSE_NUMBER(-1.234E+10, "-1.234E+10");
    TEST_PARSE_NUMBER(0.0, "1E-10000");
}

static void test_parse_string(void)
{
    TEST_PARSE_STRING("", "\"\"");
    TEST_PARSE_STRING("hello, world", "\"hello, world\"");
    TEST_PARSE_STRING("hello\0world", "\"hello\\u0000world\"");
    TEST_PARSE_STRING("\\", "\"\\\\\"");
    TEST_PARSE_STRING("/", "\"\\/\"");
    TEST_PARSE_STRING("\b", "\"\\b\"");
    TEST_PARSE_STRING("\f", "\"\\f\"");
    TEST_PARSE_STRING("\n", "\"\\n\"");
    TEST_PARSE_STRING("\r", "\"\\r\"");
    TEST_PARSE_STRING("\t", "\"\\t\"");
    TEST_PARSE_STRING("\"", "\"\\\"\"");
    /* UTF-8 */
    TEST_PARSE_STRING("\x24", "\"\\u0024\"");
    TEST_PARSE_STRING("\xC2\xA2", "\"\\u00A2\"");
    TEST_PARSE_STRING("\xE2\x82\xAC", "\"\\u20AC\"");
    TEST_PARSE_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");
    TEST_PARSE_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");
}

static void test_parse_array(void)
{
    json_value v;

    json_init(&v);
    ASSERT_EQ_INT(JSON_PARSE_OK, json_parse(&v, "[]"));
    ASSERT_EQ_INT(JSON_ARRAY, json_get_type(&v));
    ASSERT_EQ_SIZE_T(0, json_get_array_size(&v));
    json_free(&v);

    json_init(&v);
    ASSERT_EQ_INT(JSON_PARSE_OK, json_parse(&v, "[true]"));
    ASSERT_EQ_INT(JSON_TRUE, json_get_type(json_get_array_element(&v, 0)));
    ASSERT_EQ_SIZE_T(1, json_get_array_size(&v));
    json_free(&v);

    json_init(&v);
    ASSERT_EQ_INT(JSON_PARSE_OK, json_parse(&v, "[0, \"hello\", true, false, null, [1]]"));
    ASSERT_EQ_INT(JSON_ARRAY, json_get_type(&v));
    ASSERT_EQ_SIZE_T(6, json_get_array_size(&v));
    ASSERT_EQ_DOUBLE(0.0, json_get_number(json_get_array_element(&v, 0)));
    ASSERT_EQ_STRING("hello", json_get_string(json_get_array_element(&v, 1)), json_get_string_length(json_get_array_element(&v, 1)));
    ASSERT_EQ_INT(JSON_TRUE, json_get_type(json_get_array_element(&v, 2)));
    ASSERT_EQ_INT(JSON_FALSE, json_get_type(json_get_array_element(&v, 3)));
    ASSERT_EQ_INT(JSON_NULL, json_get_type(json_get_array_element(&v, 4)));
    ASSERT_EQ_INT(JSON_ARRAY, json_get_type(json_get_array_element(&v, 5)));
    ASSERT_EQ_SIZE_T(1, json_get_array_size(json_get_array_element(&v, 5)));
    ASSERT_EQ_DOUBLE(1.0, json_get_number(json_get_array_element(json_get_array_element(&v, 5), 0)));
    json_free(&v);
}

static void test_parse_object(void)
{
    json_value v;
    size_t i;

    json_init(&v);
    ASSERT_EQ_INT(JSON_PARSE_OK, json_parse(&v,
        "{"
            "\"null\" : null,"
            "\"true\" : true,"
            "\"false\" : false,"
            "\"number\" : 0,"
            "\"string\" : \"abc\","
            "\"array\" : [0, 1, 2],"
            "\"object\" : {\"0\": 0, \"1\": 1, \"2\": 2}"
        "}"
    ));
    ASSERT_EQ_INT(JSON_OBJECT, json_get_type(&v));
    ASSERT_EQ_SIZE_T(7, json_get_object_size(&v));
    ASSERT_EQ_STRING("null", json_get_object_key(&v, 0), json_get_object_key_length(&v, 0));
    ASSERT_EQ_INT(JSON_NULL, json_get_type(json_get_object_value_index(&v, 0)));
    ASSERT_EQ_POINTER(json_get_object_value_index(&v, 0), json_get_object_value(&v, "null"));

    ASSERT_EQ_STRING("true", json_get_object_key(&v, 1), json_get_object_key_length(&v, 1));
    ASSERT_EQ_INT(JSON_TRUE, json_get_type(json_get_object_value_index(&v, 1)));
    ASSERT_EQ_POINTER(json_get_object_value_index(&v, 1), json_get_object_value(&v, "true"));

    ASSERT_EQ_STRING("false", json_get_object_key(&v, 2), json_get_object_key_length(&v, 2));
    ASSERT_EQ_INT(JSON_FALSE, json_get_type(json_get_object_value_index(&v, 2)));
    ASSERT_EQ_POINTER(json_get_object_value_index(&v, 2), json_get_object_value(&v, "false"));

    ASSERT_EQ_STRING("number", json_get_object_key(&v, 3), json_get_object_key_length(&v, 3));
    ASSERT_EQ_INT(JSON_NUMBER, json_get_type(json_get_object_value_index(&v, 3)));
    ASSERT_EQ_DOUBLE(0.0, json_get_number(json_get_object_value_index(&v, 3)));
    ASSERT_EQ_POINTER(json_get_object_value_index(&v, 3), json_get_object_value(&v, "number"));

    ASSERT_EQ_STRING("string", json_get_object_key(&v, 4), json_get_object_key_length(&v, 4));
    ASSERT_EQ_INT(JSON_STRING, json_get_type(json_get_object_value_index(&v, 4)));
    ASSERT_EQ_STRING("abc", json_get_string(json_get_object_value_index(&v, 4)), json_get_string_length(json_get_object_value_index(&v, 4)));
    ASSERT_EQ_POINTER(json_get_object_value_index(&v, 4), json_get_object_value(&v, "string"));

    ASSERT_EQ_STRING("array", json_get_object_key(&v, 5), json_get_object_key_length(&v, 5));
    ASSERT_EQ_INT(JSON_ARRAY, json_get_type(json_get_object_value_index(&v, 5)));
    ASSERT_EQ_SIZE_T(3, json_get_array_size(json_get_object_value_index(&v, 5)));
    for (i = 0; i < 3; i++) {
        json_value *e = json_get_array_element(json_get_object_value_index(&v, 5), i);
        ASSERT_EQ_INT(JSON_NUMBER, json_get_type(e));
        ASSERT_EQ_DOUBLE(i, json_get_number(e));
    }
    ASSERT_EQ_POINTER(json_get_object_value_index(&v, 5), json_get_object_value(&v, "array"));

    ASSERT_EQ_STRING("object", json_get_object_key(&v, 6), json_get_object_key_length(&v, 6));
    {
        json_value *o = json_get_object_value_index(&v, 6);
        ASSERT_EQ_INT(JSON_OBJECT, json_get_type(o));
        for (i = 0; i < 3; i++) {
            ASSERT_EQ_INT('0' + i, json_get_object_key(o, i)[0]);
            ASSERT_EQ_SIZE_T(1, json_get_object_key_length(o, i));
            ASSERT_EQ_INT(JSON_NUMBER, json_get_type(json_get_object_value_index(o, i)));
            ASSERT_EQ_DOUBLE(i, json_get_number(json_get_object_value_index(o, i)));
        }
    }
    ASSERT_EQ_POINTER(json_get_object_value_index(&v, 6), json_get_object_value(&v, "object"));
    json_free(&v);
}

static void test_parse_error(void)
{
    /* Literal */
    TEST_PARSE_ERROR("");
    TEST_PARSE_ERROR("tru");
    TEST_PARSE_ERROR("FALSE");
    TEST_PARSE_ERROR("nulll");
    /* Number */
    TEST_PARSE_ERROR("+0");
    TEST_PARSE_ERROR("-");
    TEST_PARSE_ERROR("0123");
    TEST_PARSE_ERROR("0.");
    TEST_PARSE_ERROR(".123");
    TEST_PARSE_ERROR("1E");
    TEST_PARSE_ERROR("INF");
    TEST_PARSE_ERROR("NAN");
    TEST_PARSE_ERROR("0xFF");
    /* String */
    TEST_PARSE_ERROR("\"");
    TEST_PARSE_ERROR("\"abc\"\"");
    TEST_PARSE_ERROR("\"abc\\\"");
    TEST_PARSE_ERROR("\"\\v\"");
    TEST_PARSE_ERROR("\"\\0\"");
    TEST_PARSE_ERROR("\"\\x65\"");
    TEST_PARSE_ERROR("\"\0\"");
    TEST_PARSE_ERROR("\"\x1F\"");
    TEST_PARSE_ERROR("\"\\u\"");
    TEST_PARSE_ERROR("\"\\u0\"");
    TEST_PARSE_ERROR("\"\\u00\"");
    TEST_PARSE_ERROR("\"\\u000\"");
    TEST_PARSE_ERROR("\"\\u000G\"");
    TEST_PARSE_ERROR("\"\\U0000\"");
    TEST_PARSE_ERROR("\"\\uD800\"");
    TEST_PARSE_ERROR("\"\\uD8FF\"");
    TEST_PARSE_ERROR("\"\\uD800\\uDBFF\"");
    TEST_PARSE_ERROR("\"\\uD800\\uE000\"");
    /* Array */
    TEST_PARSE_ERROR("[");
    TEST_PARSE_ERROR("]");
    TEST_PARSE_ERROR("[1,]");
    TEST_PARSE_ERROR("[,]");
    TEST_PARSE_ERROR("[1 2]");
    /* Object */
    TEST_PARSE_ERROR("{");
    TEST_PARSE_ERROR("}");
    TEST_PARSE_ERROR("{\"1\"}");
    TEST_PARSE_ERROR("{\"1\":}");
    TEST_PARSE_ERROR("{1}");
    TEST_PARSE_ERROR("{:1}");
    TEST_PARSE_ERROR("{1: 1}");
    TEST_PARSE_ERROR("{null: 1}");
    TEST_PARSE_ERROR("{true: 1}");
    TEST_PARSE_ERROR("{false: 1}");
    TEST_PARSE_ERROR("{[]: 1}");
    TEST_PARSE_ERROR("{{}: 1}");
    TEST_PARSE_ERROR("{\"1\"}");
    TEST_PARSE_ERROR("{\"1\":}");
    TEST_PARSE_ERROR("{\"1\" 1}");
    TEST_PARSE_ERROR("{\"1\": 1");
    TEST_PARSE_ERROR("{\"1\": \"1}");
}

static void test_free(void)
{
    json_value v;

    json_init(&v);
    ASSERT_EQ_INT(JSON_PARSE_OK, json_parse(&v, "\"hello\""));
    ASSERT_EQ_INT(JSON_STRING, json_get_type(&v));
    json_free(&v);
    ASSERT_EQ_INT(JSON_NULL, json_get_type(&v));
}

#define TEST_JSONIFY_OK(expect, v) \
    do { \
        char *p; \
        size_t len; \
        p = json_jsonify(v, &len); \
        ASSERT_EQ_STRING(expect, p, len); \
        free(p); \
        json_free(v); \
    } while (0)

#define TEST_JSONIFY_ERROR(v) \
    do { \
        char *p; \
        p = json_jsonify(v, NULL); \
        ASSERT_EQ_POINTER(NULL, p); \
        free(p); \
        json_free(v); \
    } while (0)

#define TEST_JSONIFY_STRING(expect, string, len) \
    do { \
        json_value v; \
        json_init(&v); \
        json_set_string(&v, string, len); \
        TEST_JSONIFY_OK(expect, &v); \
    } while (0)

#define TEST_JSONIFY_NUMBER(expect, number) \
    do { \
        json_value v; \
        json_init(&v); \
        json_set_number(&v, number); \
        TEST_JSONIFY_OK(expect, &v); \
    } while (0)

#define TEST_JSONIFY_STRING_ERROR(string, len) \
    do { \
        json_value v; \
        json_init(&v); \
        json_set_string(&v, string, len); \
        TEST_JSONIFY_ERROR(&v); \
    } while (0)

static void test_jsonify_null(void)
{
    json_value v;
    json_init(&v);
    json_set_null(&v);
    TEST_JSONIFY_OK("null", &v);
}

static void test_jsonify_true(void)
{
    json_value v;
    json_init(&v);
    json_set_true(&v);
    TEST_JSONIFY_OK("true", &v);
}

static void test_jsonify_false(void)
{
    json_value v;
    json_init(&v);
    json_set_false(&v);
    TEST_JSONIFY_OK("false", &v);
}

static void test_jsonify_string(void)
{
    TEST_JSONIFY_STRING("\"\"", "", 0);
    TEST_JSONIFY_STRING("\"hello, world!\"", "hello, world!", 13);
    TEST_JSONIFY_STRING("\"\\b \\f \\n \\r \\t \\\" \\/ \\\\\"", "\b \f \n \r \t \" / \\", 15);
    TEST_JSONIFY_STRING("\"\\u0000\"", "\0", 1);
    TEST_JSONIFY_STRING("\"\\u00A2\"", "\xC2\xA2", 2);
    TEST_JSONIFY_STRING("\"\\u20AC\"", "\xE2\x82\xAC", 3);
    TEST_JSONIFY_STRING("\"\\uD834\\uDD1E\"", "\xF0\x9D\x84\x9E", 4);
}

static void test_jsonify_number(void)
{
    TEST_JSONIFY_NUMBER("1", 1.0);
    TEST_JSONIFY_NUMBER("1.234", 1.234);
    TEST_JSONIFY_NUMBER("10000000000", 1e+10);
}

static void test_jsonify_array(void)
{
    json_value v, e;

    json_init(&v);
    json_set_array(&v, 0, NULL);
    TEST_JSONIFY_OK("[]", &v);

    json_init(&v);
    json_init(&e);
    json_set_null(&e);
    json_set_array(&v, 0, &e, NULL);
    TEST_JSONIFY_OK("[null]", &v);

    json_init(&v);
    json_init(&e);
    json_set_true(&e);
    json_set_array(&v, 0, &e, NULL);
    TEST_JSONIFY_OK("[true]", &v);

    json_init(&v);
    json_init(&e);
    json_set_false(&e);
    json_set_array(&v, 0, &e, NULL);
    TEST_JSONIFY_OK("[false]", &v);

    json_init(&v);
    json_init(&e);
    json_set_string(&e, "hello, world!", 13);
    json_set_array(&v, 0, &e, NULL);
    TEST_JSONIFY_OK("[\"hello, world!\"]", &v);

    json_init(&v);
    json_init(&e);
    json_set_number(&e, 1.23);
    json_set_array(&v, 0, &e, NULL);
    TEST_JSONIFY_OK("[1.23]", &v);

    json_init(&v);
    json_init(&e);
    json_set_array(&e, 0, NULL);
    json_set_array(&v, 0, &e, NULL);
    TEST_JSONIFY_OK("[[]]", &v);

    json_init(&v);
    json_init(&e);
    json_object_append(&e, 0, NULL);
    json_set_array(&v, 0, &e, NULL);
    TEST_JSONIFY_OK("[{}]", &v);

    {
        json_value n, t, f, s, d, a, o;

        json_init(&n);
        json_set_null(&n);
        json_init(&t);
        json_set_true(&t);
        json_init(&f);
        json_set_false(&f);
        json_init(&s);
        json_set_string(&s, "hello, world!", 13);
        json_init(&d);
        json_set_number(&d, 0);
        json_init(&a);
        json_set_array(&a, 0, &d, NULL);
        json_init(&o);
        json_object_append(&o, 0, "0", 1, &d, NULL);
        json_init(&v);
        json_set_array(&v, 0, &n, &t, &f, &s, &d, &a, &o, NULL);
        TEST_JSONIFY_OK("[null, true, false, \"hello, world!\", 0, [0], {\"0\": 0}]", &v);
    }

    /* Shadow copy */
    {
        json_value s;

        json_init(&v);
        json_init(&s);
        json_set_string(&s, "Shadow copy", 11);
        json_set_array(&v, 0, &s, NULL);
        s.string[0] = 's';
        TEST_JSONIFY_OK("[\"shadow copy\"]", &v);
    }

    /* Deepcopy */
    {
        json_value *n, *t, *f, *s, *d, *a, *o;

        n = (json_value *) malloc(sizeof(json_value));
        t = (json_value *) malloc(sizeof(json_value));
        f = (json_value *) malloc(sizeof(json_value));
        s = (json_value *) malloc(sizeof(json_value));
        d = (json_value *) malloc(sizeof(json_value));
        a = (json_value *) malloc(sizeof(json_value));
        o = (json_value *) malloc(sizeof(json_value));
        json_init(n);
        json_set_null(n);
        json_init(t);
        json_set_true(t);
        json_init(f);
        json_set_false(f);
        json_init(s);
        json_set_string(s, "hello, world!", 13);
        json_init(d);
        json_set_number(d, 0);
        json_init(a);
        json_set_array(a, 0, d, NULL);
        json_init(o);
        json_object_append(o, 0, "0", 1, d, NULL);
        json_init(&v);
        json_set_array(&v, 1, n, t, f, s, d, a, o, NULL);
        json_free(n);
        json_free(t);
        json_free(f);
        json_free(s);
        json_free(d);
        json_free(a);
        json_free(o);
        free(n);
        free(t);
        free(f);
        free(s);
        free(d);
        free(a);
        free(o);
        TEST_JSONIFY_OK("[null, true, false, \"hello, world!\", 0, [0], {\"0\": 0}]", &v);
    }
}

static void test_jsonify_object(void)
{
    json_value v;

    json_init(&v);
    json_object_append(&v, 0, NULL);
    TEST_JSONIFY_OK("{}", &v);

    {
        json_value n, t, f, s, d, a, o;

        json_init(&n);
        json_set_null(&n);
        json_init(&t);
        json_set_true(&t);
        json_init(&f);
        json_set_false(&f);
        json_init(&s);
        json_set_string(&s, "hello, world!", 13);
        json_init(&d);
        json_set_number(&d, 0);
        json_init(&a);
        json_set_array(&a, 0, &d, NULL);
        json_init(&o);
        json_object_append(&o, 0, "0", (size_t) 1, &d, NULL);
        json_init(&v);
        json_object_append(&v, 0, "null", (size_t) 4, &n, "true", (size_t) 4, &t, "false", (size_t) 5, &f, "string", (size_t) 6, &s, "number", (size_t) 6, &d, "array", (size_t) 5, &a, "object", (size_t) 6, &o, NULL);
        TEST_JSONIFY_OK("{\"null\": null, \"true\": true, \"false\": false, \"string\": \"hello, world!\", \"number\": 0, \"array\": [0], \"object\": {\"0\": 0}}", &v);
    }

    {
        json_value e;

        json_init(&e);
        json_set_string(&e, "hello", 5);
        json_init(&v);
        json_object_append(&v, 1, "1", 1, &e, NULL);
        json_free(&e);
        json_init(&e);
        json_set_string(&e, "world", 5);
        json_object_append(&v, 1, "2", 1, &e, NULL);
        json_free(&e);
        TEST_JSONIFY_OK("{\"1\": \"hello\", \"2\": \"world\"}", &v);
    }
}

static void test_modify_array(void)
{
    json_value a, e, *p;

    json_init(&e);
    json_set_true(&e);
    json_init(&a);
    json_set_array(&a, 0, &e, NULL);
    json_free(&e);
    p = json_get_array_element(&a, 0);
    json_free(p);
    json_init(p);
    json_set_false(p);
    TEST_JSONIFY_OK("[false]", &a);
}

static void test_modify_object(void)
{
    json_value o, t, *v;
    char *k;

    json_init(&t);
    json_set_true(&t);
    json_init(&o);
    json_object_append(&o, 0, "true", (size_t) 4, &t, NULL);
    k = json_get_object_key(&o, 0);
    k[0] = 'T';
    v = json_get_object_value(&o, "True");
    json_free(v);
    json_init(v);
    json_set_false(v);
    TEST_JSONIFY_OK("{\"True\": false}", &o);
}

static void test_jsonify_error(void)
{
    TEST_JSONIFY_STRING_ERROR("\xC2", 1);
    TEST_JSONIFY_STRING_ERROR("\xE2\x82", 2);
    TEST_JSONIFY_STRING_ERROR("\xF0\x9D\x84", 3);
    TEST_JSONIFY_STRING_ERROR("\xFF\xFF\xFF\xFF", 4);
}

static void test(void)
{
    test_parse_true();
    test_parse_false();
    test_parse_null();
    test_parse_number();
    test_parse_string();
    test_parse_array();
    test_parse_object();
    test_parse_error();

    test_free();

    test_jsonify_true();
    test_jsonify_false();
    test_jsonify_null();
    test_jsonify_string();
    test_jsonify_number();
    test_jsonify_array();
    test_jsonify_object();
    test_modify_array();
    test_modify_object();
    test_jsonify_error();
}

int main(void)
{
    test();
    printf("=====================================================================================\n");
    printf("total:\t%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return 0;
}
