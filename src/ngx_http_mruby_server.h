/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#ifndef NGX_HTTP_MRUBY_SERVER_H
#define NGX_HTTP_MRUBY_SERVER_H

#include <ngx_http.h>
#include <mruby.h>
#include <mruby/hash.h>
#include <mruby/variable.h>

void ngx_mrb_server_class_init(mrb_state *mrb, struct RClass *calss);

#endif // NGX_HTTP_MRUBY_SERVER_H
