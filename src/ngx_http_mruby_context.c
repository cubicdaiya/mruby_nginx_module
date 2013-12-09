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

#include <mruby.h>
#include <mruby/proc.h>
#include <mruby/data.h>
#include <mruby/hash.h>

static void ngx_http_mruby_context_raise(mrb_state *mrb);

static mrb_value ngx_http_mruby_get_request_context_entry(mrb_state *mrb, mrb_value self);
static mrb_value ngx_http_mruby_set_request_context_entry(mrb_state *mrb, mrb_value self);
static mrb_value ngx_http_mruby_get_request_context(mrb_state *mrb, mrb_value self);

static void ngx_http_mruby_context_raise(mrb_state *mrb)
{
    mrb_raise(mrb,
              E_RUNTIME_ERROR, 
              "Nginx::Context is unavailable in the phase except (rewrite|access|content) phase.");
}


static mrb_value ngx_http_mruby_get_request_context_entry(mrb_state *mrb, mrb_value self)
{
    ngx_http_request_t   *r;
    ngx_http_mruby_ctx_t *ctx;
    mrb_value             key;

    r = ngx_http_mruby_get_request();
    if (r == NULL) {
        ngx_http_mruby_context_raise(mrb);
    }
    ctx = ngx_http_get_module_ctx(r, ngx_http_mruby_module);

    mrb_get_args(mrb, "o", &key);
 
    return mrb_hash_get(mrb, ctx->table, key);
}

static mrb_value ngx_http_mruby_set_request_context_entry(mrb_state *mrb, mrb_value self)
{
    ngx_http_request_t   *r;
    ngx_http_mruby_ctx_t *ctx;
    mrb_value             key, val;

    r   = ngx_http_mruby_get_request();
    if (r == NULL) {
        ngx_http_mruby_context_raise(mrb);
    }
    ctx = ngx_http_get_module_ctx(r, ngx_http_mruby_module);

    mrb_get_args(mrb, "oo", &key, &val);
    mrb_hash_set(mrb, ctx->table, key, val);

    return self;
}

static mrb_value ngx_http_mruby_get_request_context(mrb_state *mrb, mrb_value self)
{
    ngx_http_request_t   *r;
    ngx_http_mruby_ctx_t *ctx;

    r   = ngx_http_mruby_get_request();
    if (r == NULL) {
        ngx_http_mruby_context_raise(mrb);
    }
    ctx = ngx_http_get_module_ctx(r, ngx_http_mruby_module);

    return ctx->table;
}

void ngx_http_mruby_context_class_init(mrb_state *mrb, struct RClass *class)
{
    struct RClass *class_context;

    class_context = mrb_define_class_under(mrb, class, "Context", mrb->object_class);

    mrb_define_method(mrb, class_context, "[]",    ngx_http_mruby_get_request_context_entry, MRB_ARGS_ANY());
    mrb_define_method(mrb, class_context, "[]=",   ngx_http_mruby_set_request_context_entry, MRB_ARGS_ANY());
    mrb_define_method(mrb, class_context, "table", ngx_http_mruby_get_request_context,       MRB_ARGS_ANY());
}
