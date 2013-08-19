/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#include "ngx_http_mruby_module.h"
#include "ngx_http_mruby_core.h"
#include "ngx_http_mruby_request.h"
#include "ngx_http_mruby_error.h"

#include <mruby.h>
#include <mruby/proc.h>
#include <mruby/data.h>
#include <mruby/compile.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/variable.h>

#include <nginx.h>
#include <ngx_http.h>
#include <ngx_conf_file.h>
#include <ngx_log.h>
#include <ngx_buf.h>

ngx_module_t  ngx_http_mruby_module;

static mrb_value ngx_mrb_return(mrb_state *mrb, mrb_value self);
static mrb_value ngx_mrb_rputs(mrb_state *mrb, mrb_value self);
static mrb_value ngx_mrb_redirect(mrb_state *mrb, mrb_value self);

static void ngx_mrb_irep_clean(ngx_mrb_state_t *state, ngx_mrb_code_t *code)
{
    state->mrb->irep_len = code->n;
    mrb_irep_free(state->mrb, state->mrb->irep[code->n]);
    state->mrb->exc = 0;
}

ngx_int_t ngx_mrb_run_conf(ngx_conf_t *cf, ngx_mrb_state_t *state, ngx_mrb_code_t *code)
{
    mrb_run(state->mrb, mrb_proc_new(state->mrb, state->mrb->irep[code->n]), mrb_top_self(state->mrb));
    
    mrb_gc_arena_restore(state->mrb, state->ai);

    if (state->mrb->exc) {
        ngx_mrb_raise_conf_error(state, code, cf);
        return NGX_ERROR;
    }

    return NGX_OK;
}

ngx_int_t ngx_mrb_run_args(ngx_http_request_t *r, ngx_mrb_state_t *state, ngx_mrb_code_t *code, ngx_flag_t cached,
                           ngx_http_variable_value_t *args, size_t nargs, ngx_str_t *result)
{
    ngx_http_mruby_ctx_t *ctx;
    ngx_uint_t i;
    mrb_value ARGV, mrb_result;

    ctx = ngx_http_get_module_ctx(r, ngx_http_mruby_module);

    if (!cached) {
        state->ai = mrb_gc_arena_save(state->mrb);
    }

    ARGV = mrb_ary_new_capa(state->mrb, nargs);

    for (i = 0; i < nargs; i++) {
        mrb_ary_push(state->mrb, ARGV, mrb_str_new(state->mrb, (char *)args[i].data, args[i].len));
    }

    mrb_define_global_const(state->mrb, "ARGV", ARGV);

    ngx_mrb_push_request(r);
    mrb_result = mrb_run(state->mrb, mrb_proc_new(state->mrb, state->mrb->irep[code->n]), mrb_top_self(state->mrb));
    if (state->mrb->exc) {
        ngx_mrb_raise_error(state, code, r);
        mrb_gc_arena_restore(state->mrb, state->ai);
        mrb_gc_protect(state->mrb, ctx->table);
        if (!cached) {
            ngx_mrb_irep_clean(state, code);
        }
        return NGX_ERROR;
    }
    
    if (mrb_type(mrb_result) != MRB_TT_STRING) {
        mrb_result = mrb_funcall(state->mrb, mrb_result, "to_s", 0, NULL);
    }

    result->data = (u_char *)RSTRING_PTR(mrb_result);
    result->len  = ngx_strlen(result->data);

    mrb_gc_arena_restore(state->mrb, state->ai);
    mrb_gc_protect(state->mrb, ctx->table);

    if (!cached) {
        ngx_mrb_irep_clean(state, code);
    }

    return NGX_OK;
}

ngx_int_t ngx_mrb_run(ngx_http_request_t *r, ngx_mrb_state_t *state, ngx_mrb_code_t *code, ngx_flag_t cached)
{
    ngx_http_mruby_ctx_t       *ctx;
    ngx_mrb_rputs_chain_list_t *chain;

    ctx = ngx_http_get_module_ctx(r, ngx_http_mruby_module);

    ngx_mrb_push_request(r);

    if (!cached) {
        state->ai = mrb_gc_arena_save(state->mrb);
    }

    mrb_run(state->mrb, mrb_proc_new(state->mrb, state->mrb->irep[code->n]), mrb_top_self(state->mrb));

    if (state->mrb->exc) {
        ngx_mrb_raise_error(state, code, r);
    }

    mrb_gc_arena_restore(state->mrb, state->ai);
    mrb_gc_protect(state->mrb, ctx->table);
  
    if (!cached) {
        ngx_mrb_irep_clean(state, code);
    }

    if (ctx->exited) {
        return ctx->exit_code;
    }

    chain = ctx->rputs_chain;

    if (chain == NULL) {
        if (state->mrb->exc) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        
        if (r->headers_out.status >= NGX_HTTP_OK) {
            if (ctx->phase < NGX_HTTP_MRUBY_PHASE_LOG) {
                return r->headers_out.status;
            }
        }

        return NGX_DECLINED;
    }

    if (r->headers_out.status == NGX_HTTP_OK || !(*chain->last)->buf->last_buf) {
        r->headers_out.status = NGX_HTTP_OK;
        (*chain->last)->buf->last_buf = 1;
        ngx_http_send_header(r);
        ngx_http_output_filter(r, chain->out);
        return NGX_OK;
    }

    return r->headers_out.status;
}

ngx_int_t ngx_mrb_run_header_filter(ngx_http_request_t *r, ngx_mrb_state_t *state, ngx_mrb_code_t *code, ngx_flag_t cached)
{
    ngx_http_mruby_ctx_t *ctx;

    ctx = ngx_http_get_module_ctx(r, ngx_http_mruby_module);

    if (!cached) {
        state->ai = mrb_gc_arena_save(state->mrb);
    }

    ngx_mrb_push_request(r);
    mrb_run(state->mrb, mrb_proc_new(state->mrb, state->mrb->irep[code->n]), mrb_top_self(state->mrb));

    mrb_gc_arena_restore(state->mrb, state->ai);
    mrb_gc_protect(state->mrb, ctx->table);

    if (!cached) {
        ngx_mrb_irep_clean(state, code);
    }

    if (state->mrb->exc) {
        ngx_mrb_raise_error(state, code, r);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    return NGX_OK;
}

ngx_int_t ngx_mrb_run_body_filter(ngx_http_request_t *r, ngx_mrb_state_t *state, ngx_mrb_code_t *code, ngx_flag_t cached)
{
    ngx_http_mruby_ctx_t *ctx;
    mrb_value ARGV, mrb_result;

    ctx  = ngx_http_get_module_ctx(r, ngx_http_mruby_module);

    if (!cached) {
        state->ai = mrb_gc_arena_save(state->mrb);
    }

    ARGV = mrb_ary_new_capa(state->mrb, 1);

    mrb_ary_push(state->mrb, ARGV, mrb_str_new(state->mrb, (char *)ctx->filter_ctx.body, ctx->filter_ctx.body_length));
    mrb_define_global_const(state->mrb, "ARGV", ARGV);

    ngx_mrb_push_request(r);
    mrb_result = mrb_run(state->mrb, mrb_proc_new(state->mrb, state->mrb->irep[code->n]), mrb_top_self(state->mrb));
    if (state->mrb->exc) {
        ngx_mrb_raise_error(state, code, r);
        mrb_gc_arena_restore(state->mrb, state->ai);
        mrb_gc_protect(state->mrb, ctx->table);
        if (!cached) {
            ngx_mrb_irep_clean(state, code);
        }
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    
    if (mrb_type(mrb_result) != MRB_TT_STRING) {
        mrb_result = mrb_funcall(state->mrb, mrb_result, "to_s", 0, NULL);
    }

    ctx->filter_ctx.body        = (u_char *)RSTRING_PTR(mrb_result);
    ctx->filter_ctx.body_length = ngx_strlen(ctx->filter_ctx.body);

    mrb_gc_arena_restore(state->mrb, state->ai);
    mrb_gc_protect(state->mrb, ctx->table);

    if (!cached) {
        ngx_mrb_irep_clean(state, code);
    }

    return NGX_OK;
}

static mrb_value ngx_mrb_return(mrb_state *mrb, mrb_value self)
{
    ngx_mrb_rputs_chain_list_t *chain;
    ngx_http_mruby_ctx_t       *ctx;
    ngx_http_request_t         *r;
    mrb_int                     status;

    r      = ngx_mrb_get_request();
    status = NGX_HTTP_OK;

    mrb_get_args(mrb, "i", &status);

    r->headers_out.status = status;

    ctx   = ngx_http_get_module_ctx(r, ngx_http_mruby_module);
    chain = ctx->rputs_chain;

    if (chain != NULL) {
        (*chain->last)->buf->last_buf = 1;
    }

    if (r->headers_out.status == NGX_HTTP_OK && chain == NULL) {
        ngx_log_error(NGX_LOG_ERR
                      , r->connection->log
                      , 0
                      , "%s ERROR %s: Nginx.rputs is required in advance of Nginx.return(200)"
                      , MODULE_NAME
                      , __FUNCTION__
                      );
        r->headers_out.status = NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    return self;
}


static mrb_value ngx_mrb_rputs(mrb_state *mrb, mrb_value self)
{
    mrb_value                   argv;
    ngx_buf_t                  *b;
    ngx_mrb_rputs_chain_list_t *chain;
    u_char                     *str;
    ngx_str_t                   ns;
    ngx_http_request_t         *r;
    ngx_http_mruby_ctx_t       *ctx;

    r   = ngx_mrb_get_request();
    ctx = ngx_http_get_module_ctx(r, ngx_http_mruby_module);

    mrb_get_args(mrb, "o", &argv);

    if (mrb_type(argv) != MRB_TT_STRING) {
        argv = mrb_funcall(mrb, argv, "to_s", 0, NULL);
    }

    ns.data = (u_char *)RSTRING_PTR(argv);
    ns.len  = ngx_strlen(ns.data);

    if (ns.len == 0) {
        return self;
    }

    if (ctx->rputs_chain == NULL) {
        chain       = ngx_pcalloc(r->pool, sizeof(ngx_mrb_rputs_chain_list_t));
        chain->out  = ngx_alloc_chain_link(r->pool);
        chain->last = &chain->out;
    } else {
        chain = ctx->rputs_chain;
        (*chain->last)->next = ngx_alloc_chain_link(r->pool);
        chain->last = &(*chain->last)->next;
    }

    b = ngx_calloc_buf(r->pool);
    (*chain->last)->buf = b;
    (*chain->last)->next = NULL;

    if ((str = ngx_pnalloc(r->pool, ns.len + 1)) == NULL) {
        return self;
    }

    ngx_memcpy(str, ns.data, ns.len);
    str[ns.len] = '\0';
    (*chain->last)->buf->pos    = str;
    (*chain->last)->buf->last   = str + ns.len;
    (*chain->last)->buf->memory = 1;
    ctx->rputs_chain = chain;

    if (r->headers_out.content_length_n == -1) {
        r->headers_out.content_length_n += ns.len + 1;
    } else {
        r->headers_out.content_length_n += ns.len;
    }

    return self;
}

static mrb_value ngx_mrb_log(mrb_state *mrb, mrb_value self)
{   
    mrb_value          *argv;
    mrb_value           msg;
    mrb_int             argc;
    mrb_int             log_level;
    ngx_http_request_t *r;

    r = ngx_mrb_get_request();

    mrb_get_args(mrb, "*", &argv, &argc);

    if (argc != 2) {
        ngx_log_error(NGX_LOG_ERR
            , r->connection->log
            , 0
            , "%s ERROR %s: argument is not 2"
            , MODULE_NAME
            , __FUNCTION__
        );
        return self;
    }

    if (mrb_type(argv[0]) != MRB_TT_FIXNUM) {
        ngx_log_error(NGX_LOG_ERR
            , r->connection->log
            , 0
            , "%s ERROR %s: argv[0] is not integer"
            , MODULE_NAME
            , __FUNCTION__
        );
        return self;
    }

    log_level = mrb_fixnum(argv[0]);

    if (log_level < 0) {
        ngx_log_error(NGX_LOG_ERR
            , r->connection->log
            , 0
            , "%s ERROR %s: log level is not positive number"
            , MODULE_NAME
            , __FUNCTION__
        );
        return self;
    }

    if (mrb_type(argv[1]) != MRB_TT_STRING) {
        msg = mrb_funcall(mrb, argv[1], "to_s", 0, NULL);
    } else {
        msg = mrb_str_dup(mrb, argv[1]);
    }

    ngx_log_error((ngx_uint_t)log_level, r->connection->log, 0, "%s", RSTRING_PTR(msg));

    return self;
}

static mrb_value ngx_mrb_get_ngx_mruby_version(mrb_state *mrb, mrb_value self)
{   
    return mrb_str_new_cstr(mrb, MODULE_VERSION);
}

static mrb_value ngx_mrb_get_nginx_version(mrb_state *mrb, mrb_value self)
{
    return mrb_str_new_cstr(mrb, NGINX_VERSION);
}

// like Nginx rewrite keywords
// used like this:
// => http code 3xx location in browser
// => internal redirection in nginx
static mrb_value ngx_mrb_redirect(mrb_state *mrb, mrb_value self)
{
    int                   argc;
    u_char               *str;
    ngx_int_t             rc;
    mrb_value             uri, code;
    ngx_str_t             ns;
    ngx_http_request_t   *r;
    ngx_http_mruby_ctx_t *ctx;

    r    = ngx_mrb_get_request();
    ctx  = ngx_http_get_module_ctx(r, ngx_http_mruby_module);
    argc = mrb_get_args(mrb, "o|oo", &uri, &code);

    if (argc == 2) {
        rc = mrb_fixnum(code);
    } else {
        rc = NGX_HTTP_MOVED_TEMPORARILY;
    }

    if (mrb_type(uri) != MRB_TT_STRING) {
        uri = mrb_funcall(mrb, uri, "to_s", 0, NULL);
    }

    // save location uri to ns
    ns.data = (u_char *)RSTRING_PTR(uri);
    ns.len  = ngx_strlen(ns.data);

    if (ns.len == 0) {
        return mrb_nil_value();
    }

    // if uri start with scheme prefix
    // return 3xx for redirect
    // else generate a internal redirection and response to raw request
    // request.path is not changed
    if (ngx_strncmp(ns.data,    "http://",  sizeof("http://") - 1) == 0 
        || ngx_strncmp(ns.data, "https://", sizeof("https://") - 1) == 0 
        || ngx_strncmp(ns.data, "$scheme",  sizeof("$scheme") - 1) == 0) {    

        if ((str = ngx_pnalloc(r->pool, ns.len + 1)) == NULL) {
            return self;
        }

        ngx_memcpy(str, ns.data, ns.len);
        str[ns.len] = '\0';

        // build redirect location
        r->headers_out.location = ngx_list_push(&r->headers_out.headers);

        if (r->headers_out.location == NULL) {
            return self;
        }

        r->headers_out.location->hash =
                ngx_hash(ngx_hash(ngx_hash(ngx_hash(ngx_hash(ngx_hash(
                         ngx_hash('l', 'o'), 'c'), 'a'), 't'), 'i'), 'o'), 'n');

        r->headers_out.location->value.data = ns.data;
        r->headers_out.location->value.len  = ns.len;
        ngx_str_set(&r->headers_out.location->key, "Location");
        r->headers_out.status = rc;

        ctx->exited    = 1;
        ctx->exit_code = rc;
    } else {
        ctx->exited    = 1;
        ctx->exit_code = ngx_http_internal_redirect(r, &ns, &r->args);
    }

    return self;
}

void ngx_mrb_core_init(mrb_state *mrb, struct RClass *class)
{
    // status
    mrb_define_const(mrb, class, "OK",                            mrb_fixnum_value(NGX_OK));
    mrb_define_const(mrb, class, "ERROR",                         mrb_fixnum_value(NGX_ERROR));
    mrb_define_const(mrb, class, "AGAIN",                         mrb_fixnum_value(NGX_AGAIN));
    mrb_define_const(mrb, class, "BUSY",                          mrb_fixnum_value(NGX_BUSY));
    mrb_define_const(mrb, class, "DONE",                          mrb_fixnum_value(NGX_DONE));
    mrb_define_const(mrb, class, "DECLINED",                      mrb_fixnum_value(NGX_DECLINED));
    mrb_define_const(mrb, class, "ABORT",                         mrb_fixnum_value(NGX_ABORT));

    // http status
    mrb_define_const(mrb, class, "HTTP_OK",                       mrb_fixnum_value(NGX_HTTP_OK));
    mrb_define_const(mrb, class, "HTTP_CREATED",                  mrb_fixnum_value(NGX_HTTP_CREATED));
    mrb_define_const(mrb, class, "HTTP_ACCEPTED",                 mrb_fixnum_value(NGX_HTTP_ACCEPTED));
    mrb_define_const(mrb, class, "HTTP_NO_CONTENT",               mrb_fixnum_value(NGX_HTTP_NO_CONTENT));
    mrb_define_const(mrb, class, "HTTP_SPECIAL_RESPONSE",         mrb_fixnum_value(NGX_HTTP_SPECIAL_RESPONSE));
    mrb_define_const(mrb, class, "HTTP_MOVED_PERMANENTLY",        mrb_fixnum_value(NGX_HTTP_MOVED_PERMANENTLY));
    mrb_define_const(mrb, class, "HTTP_MOVED_TEMPORARILY",        mrb_fixnum_value(NGX_HTTP_MOVED_TEMPORARILY));
    mrb_define_const(mrb, class, "HTTP_SEE_OTHER",                mrb_fixnum_value(NGX_HTTP_SEE_OTHER));
    mrb_define_const(mrb, class, "HTTP_NOT_MODIFIED",             mrb_fixnum_value(NGX_HTTP_NOT_MODIFIED));
    mrb_define_const(mrb, class, "HTTP_TEMPORARY_REDIRECT",       mrb_fixnum_value(NGX_HTTP_TEMPORARY_REDIRECT));
    mrb_define_const(mrb, class, "HTTP_BAD_REQUEST",              mrb_fixnum_value(NGX_HTTP_BAD_REQUEST));
    mrb_define_const(mrb, class, "HTTP_UNAUTHORIZED",             mrb_fixnum_value(NGX_HTTP_UNAUTHORIZED));
    mrb_define_const(mrb, class, "HTTP_FORBIDDEN",                mrb_fixnum_value(NGX_HTTP_FORBIDDEN));
    mrb_define_const(mrb, class, "HTTP_NOT_FOUND",                mrb_fixnum_value(NGX_HTTP_NOT_FOUND));
    mrb_define_const(mrb, class, "HTTP_NOT_ALLOWED",              mrb_fixnum_value(NGX_HTTP_NOT_ALLOWED));
    mrb_define_const(mrb, class, "HTTP_REQUEST_TIME_OUT",         mrb_fixnum_value(NGX_HTTP_REQUEST_TIME_OUT));
    mrb_define_const(mrb, class, "HTTP_CONFLICT",                 mrb_fixnum_value(NGX_HTTP_CONFLICT));
    mrb_define_const(mrb, class, "HTTP_LENGTH_REQUIRED",          mrb_fixnum_value(NGX_HTTP_LENGTH_REQUIRED));
    mrb_define_const(mrb, class, "HTTP_PRECONDITION_FAILED",      mrb_fixnum_value(NGX_HTTP_PRECONDITION_FAILED));
    mrb_define_const(mrb, class, "HTTP_REQUEST_ENTITY_TOO_LARGE", mrb_fixnum_value(NGX_HTTP_REQUEST_ENTITY_TOO_LARGE));
    mrb_define_const(mrb, class, "HTTP_REQUEST_URI_TOO_LARGE",    mrb_fixnum_value(NGX_HTTP_REQUEST_URI_TOO_LARGE));
    mrb_define_const(mrb, class, "HTTP_UNSUPPORTED_MEDIA_TYPE",   mrb_fixnum_value(NGX_HTTP_UNSUPPORTED_MEDIA_TYPE));
    mrb_define_const(mrb, class, "HTTP_RANGE_NOT_SATISFIABLE",    mrb_fixnum_value(NGX_HTTP_RANGE_NOT_SATISFIABLE));
    mrb_define_const(mrb, class, "HTTP_CLOSE",                    mrb_fixnum_value(NGX_HTTP_CLOSE));
    mrb_define_const(mrb, class, "HTTP_NGINX_CODES",              mrb_fixnum_value(NGX_HTTP_NGINX_CODES));
    mrb_define_const(mrb, class, "HTTP_REQUEST_HEADER_TOO_LARGE", mrb_fixnum_value(NGX_HTTP_REQUEST_HEADER_TOO_LARGE));
    mrb_define_const(mrb, class, "HTTPS_CERT_ERROR",              mrb_fixnum_value(NGX_HTTPS_CERT_ERROR));
    mrb_define_const(mrb, class, "HTTPS_NO_CERT",                 mrb_fixnum_value(NGX_HTTPS_NO_CERT));
    mrb_define_const(mrb, class, "HTTP_TO_HTTPS",                 mrb_fixnum_value(NGX_HTTP_TO_HTTPS));
    mrb_define_const(mrb, class, "HTTP_CLIENT_CLOSED_REQUEST",    mrb_fixnum_value(NGX_HTTP_CLIENT_CLOSED_REQUEST));
    mrb_define_const(mrb, class, "HTTP_INTERNAL_SERVER_ERROR",    mrb_fixnum_value(NGX_HTTP_INTERNAL_SERVER_ERROR));
    mrb_define_const(mrb, class, "HTTP_NOT_IMPLEMENTED",          mrb_fixnum_value(NGX_HTTP_NOT_IMPLEMENTED));
    mrb_define_const(mrb, class, "HTTP_BAD_GATEWAY",              mrb_fixnum_value(NGX_HTTP_BAD_GATEWAY));
    mrb_define_const(mrb, class, "HTTP_SERVICE_UNAVAILABLE",      mrb_fixnum_value(NGX_HTTP_SERVICE_UNAVAILABLE));
    mrb_define_const(mrb, class, "HTTP_GATEWAY_TIME_OUT",         mrb_fixnum_value(NGX_HTTP_GATEWAY_TIME_OUT));
    mrb_define_const(mrb, class, "HTTP_INSUFFICIENT_STORAGE",     mrb_fixnum_value(NGX_HTTP_INSUFFICIENT_STORAGE));

    // method
    mrb_define_const(mrb, class, "HTTP_UNKNOWN",   mrb_fixnum_value(NGX_HTTP_UNKNOWN));
    mrb_define_const(mrb, class, "HTTP_GET",       mrb_fixnum_value(NGX_HTTP_GET));
    mrb_define_const(mrb, class, "HTTP_HEAD",      mrb_fixnum_value(NGX_HTTP_HEAD));
    mrb_define_const(mrb, class, "HTTP_PUT",       mrb_fixnum_value(NGX_HTTP_PUT));
    mrb_define_const(mrb, class, "HTTP_DELETE",    mrb_fixnum_value(NGX_HTTP_DELETE));
    mrb_define_const(mrb, class, "HTTP_MKCOL",     mrb_fixnum_value(NGX_HTTP_MKCOL));
    mrb_define_const(mrb, class, "HTTP_COPY",      mrb_fixnum_value(NGX_HTTP_COPY));
    mrb_define_const(mrb, class, "HTTP_MOVE",      mrb_fixnum_value(NGX_HTTP_MOVE));
    mrb_define_const(mrb, class, "HTTP_OPTIONS",   mrb_fixnum_value(NGX_HTTP_OPTIONS));
    mrb_define_const(mrb, class, "HTTP_PROPFIND",  mrb_fixnum_value(NGX_HTTP_PROPFIND));
    mrb_define_const(mrb, class, "HTTP_PROPPATCH", mrb_fixnum_value(NGX_HTTP_PROPPATCH));
    mrb_define_const(mrb, class, "HTTP_LOCK",      mrb_fixnum_value(NGX_HTTP_LOCK));
    mrb_define_const(mrb, class, "HTTP_UNLOCK",    mrb_fixnum_value(NGX_HTTP_UNLOCK));
    mrb_define_const(mrb, class, "HTTP_PATCH",     mrb_fixnum_value(NGX_HTTP_PATCH));
    mrb_define_const(mrb, class, "HTTP_TRACE",     mrb_fixnum_value(NGX_HTTP_TRACE));

    // error log priority
    mrb_define_const(mrb, class, "LOG_STDERR", mrb_fixnum_value(NGX_LOG_STDERR));
    mrb_define_const(mrb, class, "LOG_EMERG",  mrb_fixnum_value(NGX_LOG_EMERG));
    mrb_define_const(mrb, class, "LOG_ALERT",  mrb_fixnum_value(NGX_LOG_ALERT));
    mrb_define_const(mrb, class, "LOG_CRIT",   mrb_fixnum_value(NGX_LOG_CRIT));
    mrb_define_const(mrb, class, "LOG_ERR",    mrb_fixnum_value(NGX_LOG_ERR));
    mrb_define_const(mrb, class, "LOG_WARN",   mrb_fixnum_value(NGX_LOG_WARN));
    mrb_define_const(mrb, class, "LOG_NOTICE", mrb_fixnum_value(NGX_LOG_NOTICE));
    mrb_define_const(mrb, class, "LOG_INFO",   mrb_fixnum_value(NGX_LOG_INFO));
    mrb_define_const(mrb, class, "LOG_DEBUG",  mrb_fixnum_value(NGX_LOG_DEBUG));

    mrb_define_class_method(mrb, class, "rputs",             ngx_mrb_rputs,                 ARGS_ANY());
    mrb_define_class_method(mrb, class, "return",            ngx_mrb_return,                ARGS_ANY());
    mrb_define_class_method(mrb, class, "log",               ngx_mrb_log,                   ARGS_ANY());
    mrb_define_class_method(mrb, class, "module_version",    ngx_mrb_get_ngx_mruby_version, ARGS_NONE());
    mrb_define_class_method(mrb, class, "version",           ngx_mrb_get_nginx_version,     ARGS_NONE());
    mrb_define_class_method(mrb, class, "redirect",          ngx_mrb_redirect,              ARGS_ANY());
}
