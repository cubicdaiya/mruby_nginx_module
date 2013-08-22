/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#include "ngx_http_mruby_module.h"
#include "ngx_http_mruby_init.h"

#include "ngx_http_mruby_core.h"
#include "ngx_http_mruby_request.h"
#include "ngx_http_mruby_var.h"
#include "ngx_http_mruby_context.h"
#include "ngx_http_mruby_digest.h"
#include "ngx_http_mruby_time.h"
#include "ngx_http_mruby_base64.h"

#include <mruby.h>
#include <mruby/compile.h>

#define GC_ARENA_RESTORE mrb_gc_arena_restore(mrb, 0);

ngx_int_t ngx_mrb_class_init(mrb_state *mrb)
{
    struct RClass *class;

    class = mrb_define_module(mrb, "Nginx");

    ngx_mrb_core_init(mrb, class);          GC_ARENA_RESTORE;
    ngx_mrb_request_class_init(mrb, class); GC_ARENA_RESTORE;
    ngx_mrb_var_class_init(mrb, class);     GC_ARENA_RESTORE;
    ngx_mrb_context_class_init(mrb, class); GC_ARENA_RESTORE;
    ngx_mrb_digest_class_init(mrb, class);  GC_ARENA_RESTORE;
    ngx_mrb_time_class_init(mrb, class);    GC_ARENA_RESTORE;
    ngx_mrb_base64_class_init(mrb, class);  GC_ARENA_RESTORE;

    return NGX_OK;
}
