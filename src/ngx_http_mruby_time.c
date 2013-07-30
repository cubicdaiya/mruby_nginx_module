/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#include "ngx_http_mruby_time.h"
#include "ngx_http_mruby_request.h"

#include <mruby.h>
#include <mruby/class.h>
#include <mruby/numeric.h>

static mrb_value ngx_mrb_now(mrb_state *mrb, mrb_value self);

static mrb_value ngx_mrb_now(mrb_state *mrb, mrb_value self)
{
    return mrb_fixnum_value(ngx_time());
}

void ngx_mrb_time_class_init(mrb_state *mrb, struct RClass *class)
{
    struct RClass *class_time;

    class_time = mrb_define_class_under(mrb, class, "Time", mrb->object_class);

    mrb_define_class_method(mrb, class_time, "now", ngx_mrb_now, ARGS_ANY());
}
