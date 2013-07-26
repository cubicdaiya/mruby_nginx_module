/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#ifndef NGX_HTTP_MRUBY_VAR_H
#define NGX_HTTP_MRUBY_VAR_H

#include <ngx_http.h>
#include <mruby.h>
#include <mruby/hash.h>
#include <mruby/variable.h>
#include "ngx_http_mruby_module.h"
#include "ngx_http_mruby_request.h"

void ngx_mrb_var_class_init(mrb_state *mrb, struct RClass *calss);

#endif // NGX_HTTP_MRUBY_VAR_H
