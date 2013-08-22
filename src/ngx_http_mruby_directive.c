/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#include "ngx_http_mruby_state.h"
#include "ngx_http_mruby_directive.h"
#include "ngx_http_mruby_module.h"

#if defined(NDK) && NDK
static char *ngx_http_mruby_set_internal(ngx_conf_t *cf, ngx_command_t *cmd, void *conf, code_type_t type);
#endif

#define NGX_MRUBY_DEFINE_METHOD_HANDLER_DIRECTIVE(phase_name, phase_code, phase_handler, phase_enabled) \
char *ngx_http_mruby_##phase_name##_phase(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)               \
{                                                                                                       \
    ngx_http_mruby_main_conf_t *mmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_mruby_module);   \
    ngx_http_mruby_loc_conf_t  *mlcf;                                                                   \
    ngx_str_t *value;                                                                                   \
    ngx_mrb_code_t *code;                                                                               \
    ngx_int_t rc;                                                                                       \
    if (cmd->post == NULL) {                                                                            \
        return NGX_CONF_ERROR;                                                                          \
    }                                                                                                   \
    mlcf  = conf;                                                                                       \
    if (phase_handler != NULL) {                                                                        \
        return "is duplicated";                                                                         \
    }                                                                                                   \
    value = cf->args->elts;                                                                             \
    code  = ngx_http_mruby_mrb_code_from_file(cf->pool, &value[1]);                                     \
    if (code == NGX_CONF_UNSET_PTR) {                                                                   \
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "mrb_file(%s) open failed", value[1].data);            \
        return NGX_CONF_ERROR;                                                                          \
    }                                                                                                   \
    phase_code    = code;                                                                               \
    phase_handler = cmd->post;                                                                          \
    phase_enabled = 1;                                                                                  \
    rc = ngx_http_mruby_shared_state_compile(mmcf->state, code);                                        \
    if (rc != NGX_OK) {                                                                                 \
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "mrb_file(%s) open failed", value[1].data);            \
        return NGX_CONF_ERROR;                                                                          \
    }                                                                                                   \
    return NGX_CONF_OK;                                                                                 \
}

#define NGX_MRUBY_DEFINE_METHOD_INLINE_HANDLER_DIRECTIVE(phase_name, phase_code, phase_handler, phase_enabled) \
char *ngx_http_mruby_##phase_name##_inline_phase(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)               \
{                                                                                                              \
    ngx_http_mruby_main_conf_t *mmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_mruby_module);          \
    ngx_http_mruby_loc_conf_t  *mlcf;                                                                          \
    ngx_str_t *value;                                                                                          \
    ngx_mrb_code_t *code;                                                                                      \
    if (cmd->post == NULL) {                                                                                   \
        return NGX_CONF_ERROR;                                                                                 \
    }                                                                                                          \
    mlcf  = conf;                                                                                              \
    if (phase_handler != NULL) {                                                                               \
        return "is duplicated";                                                                                \
    }                                                                                                          \
    value = cf->args->elts;                                                                                    \
    code  = ngx_http_mruby_mrb_code_from_string(cf->pool, &value[1]);                                          \
    if (code == NGX_CONF_UNSET_PTR) {                                                                          \
        return NGX_CONF_ERROR;                                                                                 \
    }                                                                                                          \
    phase_code    = code;                                                                                      \
    phase_handler = cmd->post;                                                                                 \
    phase_enabled = 1;                                                                                         \
    ngx_http_mruby_shared_state_compile(mmcf->state, code);                                                    \
    return NGX_CONF_OK;                                                                                        \
}

NGX_MRUBY_DEFINE_METHOD_HANDLER_DIRECTIVE(rewrite, mlcf->rewrite_code, mlcf->rewrite_handler, mmcf->enabled_rewrite_handler);
NGX_MRUBY_DEFINE_METHOD_HANDLER_DIRECTIVE(access,  mlcf->access_code,  mlcf->access_handler,  mmcf->enabled_access_handler);
NGX_MRUBY_DEFINE_METHOD_HANDLER_DIRECTIVE(content, mlcf->content_code, mlcf->content_handler, mmcf->enabled_content_handler);
NGX_MRUBY_DEFINE_METHOD_HANDLER_DIRECTIVE(log,     mlcf->log_code,     mlcf->log_handler,     mmcf->enabled_log_handler);

NGX_MRUBY_DEFINE_METHOD_INLINE_HANDLER_DIRECTIVE(rewrite, mlcf->rewrite_inline_code,
                                                 mlcf->rewrite_handler, mmcf->enabled_rewrite_handler);
NGX_MRUBY_DEFINE_METHOD_INLINE_HANDLER_DIRECTIVE(access, mlcf->access_inline_code,
                                                 mlcf->access_handler, mmcf->enabled_access_handler);
NGX_MRUBY_DEFINE_METHOD_INLINE_HANDLER_DIRECTIVE(content, mlcf->content_inline_code,
                                                 mlcf->content_handler, mmcf->enabled_content_handler);
NGX_MRUBY_DEFINE_METHOD_INLINE_HANDLER_DIRECTIVE(log, mlcf->log_inline_code,
                                                 mlcf->log_handler, mmcf->enabled_log_handler);

char *ngx_http_mruby_init_phase(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{ 
    ngx_http_mruby_main_conf_t *mmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_mruby_module);
    ngx_str_t *value;
    ngx_mrb_code_t *code;
    ngx_int_t rc;

    if (cmd->post == NULL) {
        return NGX_CONF_ERROR;
    }

    if (mmcf->init_code != NULL) {
        return "is duplicated";
    }

    value = cf->args->elts;

    code = ngx_http_mruby_mrb_code_from_file(cf->pool, &value[1]);
    if (code == NGX_CONF_UNSET_PTR) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "mrb_file(%s) open failed", value[1].data);
        return NGX_CONF_ERROR;
    }
    mmcf->init_code    = code;
    mmcf->init_handler = cmd->post;
    rc = ngx_http_mruby_shared_state_compile(mmcf->state, code);
    if (rc != NGX_OK) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "mrb_file(%s) open failed", value[1].data);
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

char *ngx_http_mruby_init_inline_phase(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{ 
    ngx_http_mruby_main_conf_t *mmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_mruby_module);
    ngx_str_t *value;
    ngx_mrb_code_t *code;

    if (cmd->post == NULL) {
        return NGX_CONF_ERROR;
    }

    if (mmcf->init_code != NULL) {
        return "is duplicated";
    }

    value = cf->args->elts;

    code = ngx_http_mruby_mrb_code_from_string(cf->pool, &value[1]);
    if (code == NGX_CONF_UNSET_PTR) {
        return NGX_CONF_ERROR;
    }
    mmcf->init_code    = code;
    mmcf->init_handler = cmd->post;
    ngx_http_mruby_shared_state_compile(mmcf->state, code);

    return NGX_CONF_OK;
}

char *ngx_http_mruby_header_filter_phase(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_mruby_main_conf_t *mmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_mruby_module);
    ngx_str_t *value;
    ngx_http_mruby_loc_conf_t *mlcf = conf;
    ngx_mrb_code_t *code;
    ngx_int_t rc;

    if (cmd->post == NULL) {
        return NGX_CONF_ERROR;
    }

    if (mlcf->header_filter_handler != NULL) {
        return "is duplicated";
    }

    value = cf->args->elts;
    code  = ngx_http_mruby_mrb_code_from_file(cf->pool, &value[1]);
    if (code == NGX_CONF_UNSET_PTR) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "mrb_file(%s) open failed", value[1].data);
        return NGX_CONF_ERROR;
    }
    rc = ngx_http_mruby_shared_state_compile(mmcf->state, code);
    if (rc != NGX_OK) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "mrb_file(%s) open failed", value[1].data);
        return NGX_CONF_ERROR;
    }
    mmcf->enabled_header_filter = 1;
    mlcf->header_filter_code    = code;
    mlcf->header_filter_handler = cmd->post;

    return NGX_CONF_OK;
}

char *ngx_http_mruby_body_filter_phase(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_mruby_main_conf_t *mmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_mruby_module);
    ngx_str_t *value;
    ngx_http_mruby_loc_conf_t *mlcf = conf;
    ngx_mrb_code_t *code;
    ngx_int_t rc;

    if (cmd->post == NULL) {
        return NGX_CONF_ERROR;
    }

    if (mlcf->body_filter_handler != NULL) {
        return "is duplicated";
    }

    value = cf->args->elts;
    code  = ngx_http_mruby_mrb_code_from_file(cf->pool, &value[1]);
    if (code == NGX_CONF_UNSET_PTR) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "mrb_file(%s) open failed", value[1].data);
        return NGX_CONF_ERROR;
    }
    rc = ngx_http_mruby_shared_state_compile(mmcf->state, code);
    if (rc != NGX_OK) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "mrb_file(%s) open failed", value[1].data);
        return NGX_CONF_ERROR;
    }
    mmcf->enabled_header_filter = 1;
    mmcf->enabled_body_filter   = 1;
    mlcf->body_filter_code      = code;
    mlcf->body_filter_handler   = cmd->post;

    return NGX_CONF_OK;
}

char *ngx_http_mruby_header_filter_inline_phase(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_mruby_main_conf_t *mmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_mruby_module);
    ngx_str_t *value;
    ngx_mrb_code_t *code;
    ngx_http_mruby_loc_conf_t *mlcf = conf;

    if (cmd->post == NULL) {
        return NGX_CONF_ERROR;
    }

    if (mlcf->header_filter_handler != NULL) {
        return "is duplicated";
    }

    value = cf->args->elts;
    code  = ngx_http_mruby_mrb_code_from_string(cf->pool, &value[1]);
    if (code == NGX_CONF_UNSET_PTR) {
        return NGX_CONF_ERROR;
    }
    mlcf->header_filter_inline_code = code;
    ngx_http_mruby_shared_state_compile(mmcf->state, code);
    mmcf->enabled_header_filter = 1;
    mlcf->header_filter_handler = cmd->post;

    return NGX_CONF_OK;
}

char *ngx_http_mruby_body_filter_inline_phase(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_mruby_main_conf_t *mmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_mruby_module);
    ngx_str_t *value;
    ngx_mrb_code_t *code;
    ngx_http_mruby_loc_conf_t *mlcf = conf;

    if (cmd->post == NULL) {
        return NGX_CONF_ERROR;
    }

    if (mlcf->body_filter_handler != NULL) {
        return "is duplicated";
    }

    value = cf->args->elts;
    code  = ngx_http_mruby_mrb_code_from_string(cf->pool, &value[1]);
    if (code == NGX_CONF_UNSET_PTR) {
        return NGX_CONF_ERROR;
    }
    mlcf->body_filter_inline_code = code;
    ngx_http_mruby_shared_state_compile(mmcf->state, code);
    mmcf->enabled_header_filter = 1;
    mmcf->enabled_body_filter   = 1;
    mlcf->body_filter_handler   = cmd->post;

    return NGX_CONF_OK;
}

#if defined(NDK) && NDK

static char *ngx_http_mruby_set_internal(ngx_conf_t *cf, ngx_command_t *cmd, void *conf, code_type_t type)
{
    ngx_str_t  target;
    ngx_str_t *value;
    ndk_set_var_t filter;
    ngx_http_mruby_set_var_data_t *filter_data;
    ngx_http_mruby_main_conf_t *mmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_mruby_module);
    ngx_int_t rc;

    value  = cf->args->elts;
    target = value[1];

    filter.type = NDK_SET_VAR_MULTI_VALUE_DATA;
    filter.func = cmd->post;
    filter.size = cf->args->nelts - 3;

    filter_data = ngx_pcalloc(cf->pool, sizeof(ngx_http_mruby_set_var_data_t));
    if (filter_data == NULL) {
        return NGX_CONF_ERROR;
    }

    filter_data->state  = mmcf->state;
    filter_data->size   = filter.size;
    filter_data->script = value[2];
    if (type == NGX_MRB_CODE_TYPE_FILE) {
        filter_data->code  = ngx_http_mruby_mrb_code_from_file(cf->pool, &filter_data->script);
        rc = ngx_http_mruby_shared_state_compile(filter_data->state, filter_data->code);
        if (rc != NGX_OK) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "mrb_file(%s) open failed", filter_data->script.data);
            return NGX_CONF_ERROR;
        }
    } else {
        filter_data->code = ngx_http_mruby_mrb_code_from_string(cf->pool, &filter_data->script);
        ngx_http_mruby_shared_state_compile(filter_data->state, filter_data->code);
    } 
    if (filter_data->code == NGX_CONF_UNSET_PTR) {
        if (type == NGX_MRB_CODE_TYPE_FILE) {
            ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                               "failed to load mruby script: %s %s:%d", 
                               filter_data->script.data, __FUNCTION__, __LINE__);
        }
        return NGX_CONF_ERROR;
    }

    filter.data = filter_data;

    return ndk_set_var_multi_value_core(cf, &target, &value[3], &filter);
}

char *ngx_http_mruby_set_phase(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    return ngx_http_mruby_set_internal(cf, cmd, conf, NGX_MRB_CODE_TYPE_FILE);
}

char *ngx_http_mruby_set_inline_phase(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    return ngx_http_mruby_set_internal(cf, cmd, conf, NGX_MRB_CODE_TYPE_STRING);
}
#endif
