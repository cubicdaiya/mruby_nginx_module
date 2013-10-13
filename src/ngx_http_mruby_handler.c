/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#include <mruby.h>
#include <mruby/hash.h>
#include <mruby/variable.h>

#include "ngx_http_mruby_handler.h"
#include "ngx_http_mruby_state.h"

extern ngx_pool_t *ngx_mrb_conf_pcre_pool;

// this function is called only when initializing
ngx_int_t ngx_http_mruby_init_handler(ngx_conf_t *cf, ngx_http_mruby_main_conf_t *mmcf)
{
    ngx_int_t rc;
    ngx_mrb_conf_pcre_pool = cf->pool;
    rc = ngx_mrb_run_conf(cf, mmcf->state, mmcf->init_code);
    ngx_mrb_conf_pcre_pool = NULL;
    return rc;
}

#define NGX_MRUBY_DEFINE_METHOD_NGX_HANDLER(phase_name, handler)       \
ngx_int_t ngx_http_mruby_##phase_name##_handler(ngx_http_request_t *r) \
{                                                                      \
    ngx_http_mruby_loc_conf_t *mlcf;                                   \
    mlcf = ngx_http_get_module_loc_conf(r, ngx_http_mruby_module);     \
    if (handler == NULL) {                                             \
        return NGX_DECLINED;                                           \
    }                                                                  \
    return handler(r);                                                 \
}

NGX_MRUBY_DEFINE_METHOD_NGX_HANDLER(rewrite, mlcf->rewrite_handler);
NGX_MRUBY_DEFINE_METHOD_NGX_HANDLER(access,  mlcf->access_handler);
NGX_MRUBY_DEFINE_METHOD_NGX_HANDLER(content, mlcf->content_handler);
NGX_MRUBY_DEFINE_METHOD_NGX_HANDLER(log,     mlcf->log_handler);

#define NGX_MRUBY_DEFINE_METHOD_NGX_FILE_HANDLER(handler_name, code, context_phase)             \
ngx_int_t ngx_http_mruby_##handler_name##_file_handler(ngx_http_request_t *r)                   \
{                                                                                               \
    ngx_http_mruby_main_conf_t *mmcf = ngx_http_get_module_main_conf(r, ngx_http_mruby_module); \
    ngx_http_mruby_loc_conf_t  *mlcf = ngx_http_get_module_loc_conf(r, ngx_http_mruby_module);  \
    ngx_http_mruby_ctx_t       *ctx  = ngx_http_get_module_ctx(r, ngx_http_mruby_module);       \
    NGX_MRUBY_STATE_REINIT_IF_NOT_CACHED(                                                       \
        mlcf->cached,                                                                           \
        mmcf->state,                                                                            \
        code,                                                                                   \
        ngx_http_mruby_state_reinit_from_file                                                   \
    );                                                                                          \
    if (ctx == NULL) {                                                                          \
        if ((ctx = ngx_pcalloc(r->pool, sizeof(*ctx))) == NULL) {                               \
            ngx_log_error(NGX_LOG_ERR, r->connection->log                                       \
                , 0                                                                             \
                , "failed to allocate memory from r->pool %s:%d"                                \
                , __FUNCTION__                                                                  \
                , __LINE__                                                                      \
            );                                                                                  \
            return NGX_ERROR;                                                                   \
        }                                                                                       \
        ctx->table = mrb_hash_new(mmcf->state->mrb);                                            \
    }                                                                                           \
    ctx->phase = context_phase;                                                                 \
    ngx_http_set_ctx(r, ctx, ngx_http_mruby_module);                                            \
    return ngx_mrb_run(r, mmcf->state, code, mlcf->cached);                                     \
}

#define NGX_MRUBY_DEFINE_METHOD_NGX_INLINE_HANDLER(handler_name, code, context_phase)           \
ngx_int_t ngx_http_mruby_##handler_name##_inline_handler(ngx_http_request_t *r)                 \
{                                                                                               \
    ngx_http_mruby_main_conf_t *mmcf = ngx_http_get_module_main_conf(r, ngx_http_mruby_module); \
    ngx_http_mruby_loc_conf_t  *mlcf = ngx_http_get_module_loc_conf(r, ngx_http_mruby_module);  \
    ngx_http_mruby_ctx_t       *ctx  = ngx_http_get_module_ctx(r, ngx_http_mruby_module);       \
    if (ctx == NULL) {                                                                          \
        if ((ctx = ngx_pcalloc(r->pool, sizeof(*ctx))) == NULL) {                               \
            ngx_log_error(NGX_LOG_ERR, r->connection->log                                       \
                , 0                                                                             \
                , "failed to allocate memory from r->pool %s:%d"                                \
                , __FUNCTION__                                                                  \
                , __LINE__                                                                      \
            );                                                                                  \
            return NGX_ERROR;                                                                   \
        }                                                                                       \
        ctx->table = mrb_hash_new(mmcf->state->mrb);                                            \
    }                                                                                           \
    ctx->phase = context_phase;                                                                 \
    ngx_http_set_ctx(r, ctx, ngx_http_mruby_module);                                            \
    return ngx_mrb_run(r, mmcf->state, code, 1);                                                \
}

NGX_MRUBY_DEFINE_METHOD_NGX_FILE_HANDLER(rewrite, mlcf->rewrite_code, NGX_HTTP_MRUBY_PHASE_REWRITE)
NGX_MRUBY_DEFINE_METHOD_NGX_FILE_HANDLER(access,  mlcf->access_code,  NGX_HTTP_MRUBY_PHASE_ACCESS)
NGX_MRUBY_DEFINE_METHOD_NGX_FILE_HANDLER(content, mlcf->content_code, NGX_HTTP_MRUBY_PHASE_CONTENT)
NGX_MRUBY_DEFINE_METHOD_NGX_FILE_HANDLER(log,     mlcf->log_code,     NGX_HTTP_MRUBY_PHASE_LOG)

NGX_MRUBY_DEFINE_METHOD_NGX_INLINE_HANDLER(rewrite, mlcf->rewrite_inline_code, NGX_HTTP_MRUBY_PHASE_REWRITE)
NGX_MRUBY_DEFINE_METHOD_NGX_INLINE_HANDLER(access,  mlcf->access_inline_code,  NGX_HTTP_MRUBY_PHASE_ACCESS)
NGX_MRUBY_DEFINE_METHOD_NGX_INLINE_HANDLER(content, mlcf->content_inline_code, NGX_HTTP_MRUBY_PHASE_CONTENT)
NGX_MRUBY_DEFINE_METHOD_NGX_INLINE_HANDLER(log,     mlcf->log_inline_code,     NGX_HTTP_MRUBY_PHASE_LOG)

#if defined(NDK) && NDK
ngx_int_t ngx_http_mruby_set_handler(ngx_http_request_t *r, ngx_str_t *val,
                                     ngx_http_variable_value_t *v, void *data)
{
    ngx_http_mruby_main_conf_t    *mmcf = ngx_http_get_module_main_conf(r, ngx_http_mruby_module);
    ngx_http_mruby_loc_conf_t     *mlcf = ngx_http_get_module_loc_conf(r, ngx_http_mruby_module);
    ngx_http_mruby_ctx_t          *ctx  = ngx_http_get_module_ctx(r, ngx_http_mruby_module);
    ngx_http_mruby_set_var_data_t *filter_data;
    if (ctx == NULL) {
        if ((ctx = ngx_pcalloc(r->pool, sizeof(*ctx))) == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log
                , 0
                , "failed to allocate memory from r->pool %s:%d"
                , __FUNCTION__
                , __LINE__
            );
            return NGX_ERROR;
        }
        ctx->table = mrb_hash_new(mmcf->state->mrb);
    }
    ctx->phase = NGX_HTTP_MRUBY_PHASE_SET;
    ngx_http_set_ctx(r, ctx, ngx_http_mruby_module);

    filter_data = data;
    if (!mlcf->cached && ngx_http_mruby_state_reinit_from_file(filter_data->state, filter_data->code)) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to load mruby script: %s %s:%d", 
                      filter_data->script.data, __FUNCTION__, __LINE__);
        return NGX_ERROR;
    }

    return ngx_mrb_run_args(r, filter_data->state, filter_data->code, mlcf->cached, v, filter_data->size, val);
}

ngx_int_t ngx_http_mruby_set_inline_handler(ngx_http_request_t *r, ngx_str_t *val,
                                            ngx_http_variable_value_t *v, void *data)
{
    ngx_http_mruby_main_conf_t    *mmcf = ngx_http_get_module_main_conf(r, ngx_http_mruby_module);
    ngx_http_mruby_ctx_t          *ctx  = ngx_http_get_module_ctx(r, ngx_http_mruby_module);
    ngx_http_mruby_set_var_data_t *filter_data;
    if (ctx == NULL) {
        if ((ctx = ngx_pcalloc(r->pool, sizeof(*ctx))) == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log
                , 0
                , "failed to allocate memory from r->pool %s:%d"
                , __FUNCTION__
                , __LINE__
            );
            return NGX_ERROR;
        }
        ctx->table = mrb_hash_new(mmcf->state->mrb);
    }
    ctx->phase = NGX_HTTP_MRUBY_PHASE_SET;
    ngx_http_set_ctx(r, ctx, ngx_http_mruby_module);
    filter_data = data;
    return ngx_mrb_run_args(r, filter_data->state, filter_data->code, 1, v, filter_data->size, val);
}
#endif
