/**
 *  Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
 *  Copyright (c) ngx_mruby developers 2012-
 *
 *  Use and distribution licensed under the MIT license.
 *  See LICENSE for full text.
 *
 */

#include <ngx_config.h>
#include <ngx_http.h>
#include <ngx_conf_file.h>
#include <nginx.h>

#include "ngx_http_mruby_module.h"
#include "ngx_http_mruby_handler.h"
#include "ngx_http_mruby_filter_handler.h"
#include "ngx_http_mruby_directive.h"
#include "ngx_http_mruby_state.h"

static ngx_http_request_t *ngx_mruby_request;

#define NGX_MRUBY_MERGE_CODE(prev_code, conf_code)     \
    if (prev_code == NGX_CONF_UNSET_PTR) {             \
        prev_code = conf_code;                         \
    }

// set conf
static void *ngx_http_mruby_create_main_conf(ngx_conf_t *cf);
static char *ngx_http_mruby_init_main_conf(ngx_conf_t *cf, void *conf);
static void *ngx_http_mruby_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_mruby_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

// set init function
static ngx_int_t ngx_http_mruby_preinit(ngx_conf_t *cf);
static ngx_int_t ngx_http_mruby_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_mruby_handler_init(ngx_http_core_main_conf_t *cmcf, ngx_http_mruby_main_conf_t *mmcf);

static ngx_command_t ngx_http_mruby_commands[] = {

    { ngx_string("mruby_cache"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_mruby_loc_conf_t, cached),
      NULL 
    },

    { ngx_string("mruby_require"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_http_mruby_require,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL
    },

    { ngx_string("mruby_init_code"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_http_mruby_init_inline_phase,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      ngx_http_mruby_init_handler
    },

    { ngx_string("mruby_init"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_http_mruby_init_phase,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      ngx_http_mruby_init_handler
    },

    { ngx_string("mruby_rewrite_handler"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_http_mruby_rewrite_phase,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      ngx_http_mruby_rewrite_file_handler
    },
 
    { ngx_string("mruby_access_handler"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_http_mruby_access_phase,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      ngx_http_mruby_access_file_handler
    },
 
    { ngx_string("mruby_content_handler"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_http_mruby_content_phase,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      ngx_http_mruby_content_file_handler
    },
 
    { ngx_string("mruby_log_handler"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_http_mruby_log_phase,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      ngx_http_mruby_log_file_handler
    },
 
    { ngx_string("mruby_rewrite_handler_code"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_http_mruby_rewrite_inline_phase,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      ngx_http_mruby_rewrite_inline_handler },

    { ngx_string("mruby_access_handler_code"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_http_mruby_access_inline_phase,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      ngx_http_mruby_access_inline_handler },

    { ngx_string("mruby_content_handler_code"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_http_mruby_content_inline_phase,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      ngx_http_mruby_content_inline_handler },
 
    { ngx_string("mruby_log_handler_code"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_http_mruby_log_inline_phase,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      ngx_http_mruby_log_inline_handler },

    { ngx_string("mruby_header_filter"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_http_mruby_header_filter_phase,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      ngx_http_mruby_header_filter_handler },

    { ngx_string("mruby_header_filter_code"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_http_mruby_header_filter_inline_phase,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      ngx_http_mruby_header_filter_inline_handler },

    { ngx_string("mruby_body_filter"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_http_mruby_body_filter_phase,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      ngx_http_mruby_body_filter_handler },

    { ngx_string("mruby_body_filter_code"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1,
      ngx_http_mruby_body_filter_inline_phase,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      ngx_http_mruby_body_filter_inline_handler },

#if defined(NDK) && NDK
    { ngx_string("mruby_set"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_2MORE,
      ngx_http_mruby_set_phase,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      ngx_http_mruby_set_handler },

    { ngx_string("mruby_set_code"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_CONF_2MORE,
      ngx_http_mruby_set_inline_phase,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      ngx_http_mruby_set_inline_handler },
#endif
 
    ngx_null_command
};
 
static ngx_http_module_t ngx_http_mruby_module_ctx = {
    ngx_http_mruby_preinit,          /* preconfiguration */
    ngx_http_mruby_init,             /* postconfiguration */
 
    ngx_http_mruby_create_main_conf, /* create main configuration */
    ngx_http_mruby_init_main_conf,   /* init main configuration */
 
    NULL,                            /* create server configuration */
    NULL,                            /* merge server configuration */
 
    ngx_http_mruby_create_loc_conf,  /* create location configuration */
    ngx_http_mruby_merge_loc_conf    /* merge location configuration */
};
 
ngx_module_t ngx_http_mruby_module = {
    NGX_MODULE_V1,
    &ngx_http_mruby_module_ctx,    /* module context */
    ngx_http_mruby_commands,       /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};

static void *ngx_http_mruby_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_mruby_main_conf_t *mmcf;

    mmcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_mruby_main_conf_t));
    if (mmcf == NULL) {
        return NULL;
    }
    mmcf->state = ngx_pcalloc(cf->pool, sizeof(ngx_mrb_state_t));
    if (mmcf->state == NULL) {
        return NULL;
    }

    return mmcf;
}

static char *ngx_http_mruby_init_main_conf(ngx_conf_t *cf, void *conf)
{
    return NGX_CONF_OK;
}

static void *ngx_http_mruby_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_mruby_loc_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_mruby_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->rewrite_code = NGX_CONF_UNSET_PTR;
    conf->access_code  = NGX_CONF_UNSET_PTR;
    conf->content_code = NGX_CONF_UNSET_PTR;
    conf->log_code     = NGX_CONF_UNSET_PTR;

    conf->rewrite_inline_code = NGX_CONF_UNSET_PTR;
    conf->access_inline_code  = NGX_CONF_UNSET_PTR;
    conf->content_inline_code = NGX_CONF_UNSET_PTR;
    conf->log_inline_code     = NGX_CONF_UNSET_PTR;

    conf->header_filter_code        = NGX_CONF_UNSET_PTR;
    conf->body_filter_code          = NGX_CONF_UNSET_PTR;
    conf->header_filter_inline_code = NGX_CONF_UNSET_PTR;
    conf->body_filter_inline_code   = NGX_CONF_UNSET_PTR;

    conf->cached = NGX_CONF_UNSET;

    return conf;
}

static char *ngx_http_mruby_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_mruby_loc_conf_t *prev = parent;
    ngx_http_mruby_loc_conf_t *conf = child;

    NGX_MRUBY_MERGE_CODE(prev->rewrite_code, conf->rewrite_code);
    NGX_MRUBY_MERGE_CODE(prev->access_code,  conf->access_code);
    NGX_MRUBY_MERGE_CODE(prev->content_code, conf->content_code);
    NGX_MRUBY_MERGE_CODE(prev->log_code,     conf->log_code);

    NGX_MRUBY_MERGE_CODE(prev->rewrite_inline_code, conf->rewrite_inline_code);
    NGX_MRUBY_MERGE_CODE(prev->access_inline_code,  conf->access_inline_code);
    NGX_MRUBY_MERGE_CODE(prev->content_inline_code, conf->content_inline_code);
    NGX_MRUBY_MERGE_CODE(prev->log_inline_code,     conf->log_inline_code);

    NGX_MRUBY_MERGE_CODE(prev->header_filter_code,        conf->header_filter_code);
    NGX_MRUBY_MERGE_CODE(prev->body_filter_code,          conf->body_filter_code);
    NGX_MRUBY_MERGE_CODE(prev->header_filter_inline_code, conf->header_filter_inline_code);
    NGX_MRUBY_MERGE_CODE(prev->body_filter_inline_code,   conf->body_filter_inline_code);

    // mruby_cache is on by default
    ngx_conf_merge_value(conf->cached, prev->cached, 1);

    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_mruby_preinit(ngx_conf_t *cf)
{
    ngx_http_mruby_main_conf_t *mmcf;

    mmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_mruby_module);
    ngx_http_mruby_shared_state_init(mmcf->state);

    return NGX_OK;
}

static ngx_int_t ngx_http_mruby_init(ngx_conf_t *cf)
{
    ngx_http_core_main_conf_t  *cmcf;
    ngx_http_mruby_main_conf_t *mmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    mmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_mruby_module);

    ngx_mrb_init_request();

    if (ngx_http_mruby_handler_init(cmcf, mmcf) != NGX_OK) {
        return NGX_ERROR;
    }

    if (mmcf->enabled_header_filter) {
        ngx_http_mruby_header_filter_init();
    }
    if (mmcf->enabled_body_filter) {
        ngx_http_mruby_body_filter_init();
    }

    if (mmcf->init_handler) {
        return mmcf->init_handler(cf, mmcf);
    }

    return NGX_OK;
}

static ngx_int_t ngx_http_mruby_handler_init(ngx_http_core_main_conf_t *cmcf, ngx_http_mruby_main_conf_t *mmcf)
{
    ngx_int_t i;
    ngx_http_handler_pt *h;
    ngx_http_phases phase;
    ngx_http_phases phases[] = {
        NGX_HTTP_REWRITE_PHASE,
        NGX_HTTP_ACCESS_PHASE,
        NGX_HTTP_CONTENT_PHASE,
        NGX_HTTP_LOG_PHASE,
    };
    ngx_int_t phases_c;

    phases_c = sizeof(phases) / sizeof(ngx_http_phases);
    for (i=0;i<phases_c;i++) {
        phase = phases[i];
        switch (phase) {
        case NGX_HTTP_REWRITE_PHASE:
            if (mmcf->enabled_rewrite_handler) {
                h = ngx_array_push(&cmcf->phases[phase].handlers);
                if (h == NULL) {
                    return NGX_ERROR;
                }
                *h = ngx_http_mruby_rewrite_handler;
            }
            break;
        case NGX_HTTP_ACCESS_PHASE:
            if (mmcf->enabled_access_handler) {
                h = ngx_array_push(&cmcf->phases[phase].handlers);
                if (h == NULL) {
                    return NGX_ERROR;
                }
                *h = ngx_http_mruby_access_handler;
            }
            break;
        case NGX_HTTP_CONTENT_PHASE:
            if (mmcf->enabled_content_handler) {
                h = ngx_array_push(&cmcf->phases[phase].handlers);
                if (h == NULL) {
                    return NGX_ERROR;
                }
                *h = ngx_http_mruby_content_handler;
            }
            break;
        case NGX_HTTP_LOG_PHASE:
            if (mmcf->enabled_log_handler) {
                h = ngx_array_push(&cmcf->phases[phase].handlers);
                if (h == NULL) {
                    return NGX_ERROR;
                }
                *h = ngx_http_mruby_log_handler;
            }
            break;
        default:
            // not through
            break;
        }
    }

    return NGX_OK;
}

void ngx_mrb_init_request(void)
{
    ngx_mruby_request = NULL;
}

void ngx_mrb_push_request(ngx_http_request_t *r)
{
    ngx_mruby_request = r;
}

ngx_http_request_t *ngx_mrb_get_request(void)
{
    return ngx_mruby_request;
}
