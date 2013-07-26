/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#ifndef NGX_HTTP_MRUBY_INIT_H
#define NGX_HTTP_MRUBY_INIT_H

#include <ngx_http.h>
#include <mruby.h>
#include "ngx_http_mruby_core.h"

ngx_int_t ngx_mrb_class_init(mrb_state *mrb);

#endif // NGX_HTTP_MRUBY_INIT_H
