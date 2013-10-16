/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#include "ngx_http_mruby_module.h"
#include "ngx_http_mruby_class.h"

#include <nginx.h>
#include <ngx_http.h>

#include <mruby.h>
#include <mruby/string.h>
#include <mruby/class.h>
#include <mruby/hash.h>
#include <mruby/variable.h>

#define NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_MEMBER_STR(method_suffix, member)        \
static mrb_value ngx_http_mruby_get_##method_suffix(mrb_state *mrb, mrb_value self);     \
static mrb_value ngx_http_mruby_get_##method_suffix(mrb_state *mrb, mrb_value self)      \
{                                                                                        \
    ngx_http_request_t *r = ngx_http_mruby_get_request();                                \
    return mrb_str_new(mrb, (const char *)member.data, member.len);                      \
}

#define NGX_MRUBY_DEFINE_METHOD_NGX_SET_REQUEST_MEMBER_STR(method_suffix, member)        \
static mrb_value ngx_http_mruby_set_##method_suffix(mrb_state *mrb, mrb_value self);     \
static mrb_value ngx_http_mruby_set_##method_suffix(mrb_state *mrb, mrb_value self)      \
{                                                                                        \
    mrb_value arg;                                                                       \
    u_char *str;                                                                         \
    ngx_http_request_t *r;                                                               \
    mrb_get_args(mrb, "o", &arg);                                                        \
    if (mrb_nil_p(arg)) {                                                                \
        return self;                                                                     \
    }                                                                                    \
    str = (u_char *)RSTRING_PTR(arg);                                                    \
    r   = ngx_http_mruby_get_request();                                                  \
    member.len  = RSTRING_LEN(arg);                                                      \
    member.data = (u_char *)str;                                                         \
    return self;                                                                         \
}

#define NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_HEADERS_HASH(direction)                                 \
static mrb_value ngx_http_mruby_get_request_headers_##direction##_hash(mrb_state *mrb, mrb_value self); \
static mrb_value ngx_http_mruby_get_request_headers_##direction##_hash(mrb_state *mrb, mrb_value self)  \
{                                                                                                       \
    ngx_list_part_t    *part;                                                                           \
    ngx_table_elt_t    *header;                                                                         \
    ngx_http_request_t *r;                                                                              \
    ngx_uint_t          i;                                                                              \
    mrb_value           hash;                                                                           \
    mrb_value           key;                                                                            \
    mrb_value           value;                                                                          \
    r      = ngx_http_mruby_get_request();                                                              \
    hash   = mrb_hash_new(mrb);                                                                         \
    part   = &(r->headers_##direction.headers.part);                                                    \
    header = part->elts;                                                                                \
    for (i = 0; /* void */; i++) {                                                                      \
        if (i >= part->nelts) {                                                                         \
            if (part->next == NULL) {                                                                   \
                break;                                                                                  \
            }                                                                                           \
            part   = part->next;                                                                        \
            header = part->elts;                                                                        \
            i      = 0;                                                                                 \
        }                                                                                               \
        key   = mrb_str_new(mrb, (const char *)header[i].key.data,   header[i].key.len);                \
        value = mrb_str_new(mrb, (const char *)header[i].value.data, header[i].value.len);              \
        mrb_hash_set(mrb, hash, key, value);                                                            \
    }                                                                                                   \
    return hash;                                                                                        \
}

static mrb_value ngx_http_mruby_get_request_header(mrb_state *mrb, ngx_list_t *headers);
static mrb_value ngx_http_mruby_get_request_headers_in(mrb_state *mrb, mrb_value self);
static mrb_value ngx_http_mruby_get_request_headers_out(mrb_state *mrb, mrb_value self);
static ngx_int_t ngx_http_mruby_set_request_header(mrb_state *mrb, ngx_list_t *headers);
static mrb_value ngx_http_mruby_set_request_headers_in(mrb_state *mrb, mrb_value self);
static mrb_value ngx_http_mruby_set_request_headers_out(mrb_state *mrb, mrb_value self);

static mrb_value ngx_http_mruby_get_request_var(mrb_state *mrb, mrb_value self);

// request member getter
NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_MEMBER_STR(request_request_line, r->request_line);
NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_MEMBER_STR(request_uri,          r->uri);
NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_MEMBER_STR(request_unparsed_uri, r->unparsed_uri);
NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_MEMBER_STR(request_method,       r->method_name);
NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_MEMBER_STR(request_protocol,     r->http_protocol);
NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_MEMBER_STR(request_args,         r->args);

// request member setter
NGX_MRUBY_DEFINE_METHOD_NGX_SET_REQUEST_MEMBER_STR(request_request_line, r->request_line);
NGX_MRUBY_DEFINE_METHOD_NGX_SET_REQUEST_MEMBER_STR(request_uri,          r->uri);
NGX_MRUBY_DEFINE_METHOD_NGX_SET_REQUEST_MEMBER_STR(request_unparsed_uri, r->unparsed_uri);
NGX_MRUBY_DEFINE_METHOD_NGX_SET_REQUEST_MEMBER_STR(request_method,       r->method_name);
NGX_MRUBY_DEFINE_METHOD_NGX_SET_REQUEST_MEMBER_STR(request_protocol,     r->http_protocol);
NGX_MRUBY_DEFINE_METHOD_NGX_SET_REQUEST_MEMBER_STR(request_args,         r->args);

NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_HEADERS_HASH(in);
NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_HEADERS_HASH(out);

// TODO:this declation should be moved to headers_(in|out)
NGX_MRUBY_DEFINE_METHOD_NGX_GET_REQUEST_MEMBER_STR(content_type, r->headers_out.content_type);
NGX_MRUBY_DEFINE_METHOD_NGX_SET_REQUEST_MEMBER_STR(content_type, r->headers_out.content_type);


static mrb_value ngx_http_mruby_get_request_header(mrb_state *mrb, ngx_list_t *headers)
{
    mrb_value        mrb_key;
    u_char          *key;
    ngx_uint_t       i;
    ngx_list_part_t *part;
    ngx_table_elt_t *header;

    mrb_get_args(mrb, "o", &mrb_key);

    key    = (u_char *)RSTRING_PTR(mrb_key);
    part   = &headers->part;
    header = part->elts;

    /* TODO:optimize later(linear-search is slow) */
    for (i = 0; /* void */; i++) {
        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part   = part->next;
            header = part->elts;
            i      = 0;
        }

        if (ngx_strncasecmp(key, header[i].key.data, header[i].key.len) == 0) {
            return mrb_str_new(mrb, (const char *)header[i].value.data, header[i].value.len);
        }
    }

    return mrb_nil_value();
}

static ngx_int_t ngx_http_mruby_set_request_header(mrb_state *mrb, ngx_list_t *headers)
{
    mrb_value           mrb_key, mrb_val;
    u_char             *key, *val;
    size_t              key_len, val_len;
    ngx_uint_t          i;
    ngx_list_part_t    *part;
    ngx_table_elt_t    *header;
    ngx_table_elt_t    *new_header;

    mrb_get_args(mrb, "oo", &mrb_key, &mrb_val);

    key     = (u_char *)RSTRING_PTR(mrb_key);
    val     = (u_char *)RSTRING_PTR(mrb_val);
    key_len = RSTRING_LEN(mrb_key);
    val_len = RSTRING_LEN(mrb_val);
    part    = &headers->part;
    header  = part->elts;

    /* TODO:optimize later(linear-search is slow) */
    for (i = 0; /* void */; i++) {
        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part   = part->next;
            header = part->elts;
            i      = 0;
        }

        if (ngx_strncasecmp(key, header[i].key.data, header[i].key.len) == 0) {
            header[i].value.data = val;
            header[i].value.len  = val_len;
            return NGX_OK;
        }
    }

    new_header = ngx_list_push(headers);
    if (new_header == NULL) {
        return NGX_ERROR;
    }
    new_header->hash       = 1;
    new_header->key.data   = key;
    new_header->key.len    = key_len;
    new_header->value.data = val;
    new_header->value.len  = val_len;

    return NGX_OK;
}

static mrb_value ngx_http_mruby_get_request_headers_in(mrb_state *mrb, mrb_value self)
{
    ngx_http_request_t *r;
    r = ngx_http_mruby_get_request();
    return ngx_http_mruby_get_request_header(mrb, &r->headers_in.headers);
}

static mrb_value ngx_http_mruby_get_request_headers_out(mrb_state *mrb, mrb_value self)
{
    ngx_http_request_t *r;
    r = ngx_http_mruby_get_request();
    return ngx_http_mruby_get_request_header(mrb, &r->headers_out.headers);
}

static mrb_value ngx_http_mruby_set_request_headers_in(mrb_state *mrb, mrb_value self)
{
    ngx_http_request_t *r;
    r = ngx_http_mruby_get_request();
    ngx_http_mruby_set_request_header(mrb, &r->headers_in.headers);
    return self;
}

static mrb_value ngx_http_mruby_set_request_headers_out(mrb_state *mrb, mrb_value self)
{
    ngx_http_request_t *r;
    r = ngx_http_mruby_get_request();
    ngx_http_mruby_set_request_header(mrb, &r->headers_out.headers);
    return self;
}

static mrb_value ngx_http_mruby_get_request_var(mrb_state *mrb, mrb_value self)
{
    const char    *iv_var_str = "@iv_var";
    mrb_value     iv_var;
    struct RClass *class_var, *class_ngx;

    iv_var = mrb_iv_get(mrb, self, mrb_intern(mrb, iv_var_str));

    if (mrb_nil_p(iv_var)) {
        class_ngx = mrb_class_get(mrb, "Nginx");
        class_var = (struct RClass*)mrb_class_ptr(mrb_const_get(mrb, mrb_obj_value(class_ngx), mrb_intern_cstr(mrb, "Var")));
        iv_var    = mrb_obj_new(mrb, class_var, 0, NULL);
        mrb_iv_set(mrb, self, mrb_intern(mrb, iv_var_str), iv_var);
    }

    return iv_var;
}

void ngx_http_mruby_request_class_init(mrb_state *mrb, struct RClass *class)
{
    struct RClass *class_request;
    struct RClass *class_headers_in;
    struct RClass *class_headers_out;

    class_request = mrb_define_class_under(mrb, class, "Request", mrb->object_class);

    mrb_define_method(mrb, class_request, "content_type=", ngx_http_mruby_set_content_type,         ARGS_ANY());
    mrb_define_method(mrb, class_request, "content_type",  ngx_http_mruby_get_content_type,         ARGS_NONE());
    mrb_define_method(mrb, class_request, "request_line",  ngx_http_mruby_get_request_request_line, ARGS_NONE());
    mrb_define_method(mrb, class_request, "request_line=", ngx_http_mruby_set_request_request_line, ARGS_ANY());
    mrb_define_method(mrb, class_request, "uri",           ngx_http_mruby_get_request_uri,          ARGS_NONE());
    mrb_define_method(mrb, class_request, "uri=",          ngx_http_mruby_set_request_uri,          ARGS_ANY());
    mrb_define_method(mrb, class_request, "unparsed_uri",  ngx_http_mruby_get_request_unparsed_uri, ARGS_NONE());
    mrb_define_method(mrb, class_request, "unparsed_uri=", ngx_http_mruby_set_request_unparsed_uri, ARGS_ANY());
    mrb_define_method(mrb, class_request, "method",        ngx_http_mruby_get_request_method,       ARGS_NONE());
    mrb_define_method(mrb, class_request, "method=",       ngx_http_mruby_set_request_method,       ARGS_ANY());
    mrb_define_method(mrb, class_request, "protocol",      ngx_http_mruby_get_request_protocol,     ARGS_NONE());
    mrb_define_method(mrb, class_request, "protocol=",     ngx_http_mruby_set_request_protocol,     ARGS_ANY());
    mrb_define_method(mrb, class_request, "args",          ngx_http_mruby_get_request_args,         ARGS_NONE());
    mrb_define_method(mrb, class_request, "args=",         ngx_http_mruby_set_request_args,         ARGS_ANY());
    mrb_define_method(mrb, class_request, "var",           ngx_http_mruby_get_request_var,          ARGS_NONE());

    class_headers_in = mrb_define_class_under(mrb, class, "Headers_in", mrb->object_class);

    mrb_define_method(mrb, class_headers_in, "[]",              ngx_http_mruby_get_request_headers_in,      ARGS_ANY());
    mrb_define_method(mrb, class_headers_in, "[]=",             ngx_http_mruby_set_request_headers_in,      ARGS_ANY());
    mrb_define_method(mrb, class_headers_in, "headers_in_hash", ngx_http_mruby_get_request_headers_in_hash, ARGS_ANY());

    class_headers_out = mrb_define_class_under(mrb, class, "Headers_out", mrb->object_class);

    mrb_define_method(mrb, class_headers_out, "[]",               ngx_http_mruby_get_request_headers_out,      ARGS_ANY());
    mrb_define_method(mrb, class_headers_out, "[]=",              ngx_http_mruby_set_request_headers_out,      ARGS_ANY());
    mrb_define_method(mrb, class_headers_out, "headers_out_hash", ngx_http_mruby_get_request_headers_out_hash, ARGS_ANY());
}
