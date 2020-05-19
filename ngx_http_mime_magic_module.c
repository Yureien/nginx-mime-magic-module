/**
 * BSD 2-Clause License
 *
 * Copyright (c) 2020, Soham Sen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <magic.h>

static ngx_int_t ngx_http_mime_magic_init(ngx_conf_t *cf);
static void *ngx_http_mime_magic_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_mime_magic_merge_conf(ngx_conf_t *cf, void *parent, void *child);

typedef struct
{
    ngx_flag_t enable;
    ngx_str_t magic_db;
} ngx_http_mime_magic_conf_t;

magic_t magic_cookie;

static ngx_command_t ngx_http_mime_magic_commands[] = {

    {ngx_string("mime_magic"),
     NGX_HTTP_LOC_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_MAIN_CONF | NGX_CONF_FLAG,
     ngx_conf_set_flag_slot,
     NGX_HTTP_LOC_CONF_OFFSET,
     offsetof(ngx_http_mime_magic_conf_t, enable),
     NULL},

    {ngx_string("mime_magic_db"),
     NGX_HTTP_LOC_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
     ngx_conf_set_str_slot,
     NGX_HTTP_LOC_CONF_OFFSET,
     offsetof(ngx_http_mime_magic_conf_t, magic_db),
     NULL},

    ngx_null_command /* command termination */
};

/* The module context. */
static ngx_http_module_t ngx_http_mime_magic_module_ctx = {
    NULL,                     /* preconfiguration */
    ngx_http_mime_magic_init, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    ngx_http_mime_magic_create_loc_conf, /* create location configuration */
    ngx_http_mime_magic_merge_conf       /* merge location configuration */
};

/* Module definition. */
ngx_module_t ngx_http_mime_magic_module = {
    NGX_MODULE_V1,
    &ngx_http_mime_magic_module_ctx, /* module context */
    ngx_http_mime_magic_commands,    /* module directives */
    NGX_HTTP_MODULE,                 /* module type */
    NULL,                            /* init master */
    NULL,                            /* init module */
    NULL,                            /* init process */
    NULL,                            /* init thread */
    NULL,                            /* exit thread */
    NULL,                            /* exit process */
    NULL,                            /* exit master */
    NGX_MODULE_V1_PADDING};

static ngx_http_output_header_filter_pt ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt ngx_http_next_body_filter;

static ngx_int_t ngx_http_mime_magic_header_filter(ngx_http_request_t *r)
{
    ngx_http_mime_magic_conf_t *conf;
    conf = ngx_http_get_module_loc_conf(r, ngx_http_mime_magic_module);

    if (conf == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ERROR: null configuration. Module mime_magic disabled.");

        return ngx_http_next_header_filter(r);
    }

    if (!conf->enable)
        return ngx_http_next_header_filter(r);

    if (magic_cookie == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ERROR: magic_cookie not initialized. Module mime_magic disabled.");

        return ngx_http_next_header_filter(r);
    }

    return 0; // Do nothing, handle headers in ngx_http_mime_magic_body_filter
    // Don't ask why it is 0. I searched everywhere but couldn't find what exactly return codes should be.
}

static ngx_int_t ngx_http_mime_magic_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    ngx_http_mime_magic_conf_t *conf;
    conf = ngx_http_get_module_loc_conf(r, ngx_http_mime_magic_module);

    if (conf == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ERROR: null configuration. Module mime_magic disabled.");

        return ngx_http_next_body_filter(r, in);
    }

    if (!conf->enable)
        return ngx_http_next_body_filter(r, in);

    if (r->main != r) // Only main request
        return ngx_http_next_body_filter(r, in);

    if (magic_cookie == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ERROR: magic_cookie not initialized. Module mime_magic disabled.");

        return ngx_http_next_body_filter(r, in);
    }

    if (in == NULL || in->buf == NULL || in->buf->file == NULL)
    {
        // Do not operate on empty chains OR
        // Do not operate on null buffers OR
        // Do not operate on requests that aren't served from file, ex: autoindex
        ngx_http_next_header_filter(r);
        return ngx_http_next_body_filter(r, in);
    }

    if (in->buf->file_pos != 0)
    {
        // Do not operate on buffers other than first (like when HTTP 204 or multiple buffers)
        ngx_http_next_header_filter(r);
        return ngx_http_next_body_filter(r, in);
    }

    const char *mime = magic_descriptor(magic_cookie, in->buf->file->fd);

    r->headers_out.content_type.data = (u_char *)mime;
    r->headers_out.content_type.len = ngx_strlen((u_char *)mime);

    ngx_http_next_header_filter(r);

    return ngx_http_next_body_filter(r, in);
}

static void *ngx_http_mime_magic_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_mime_magic_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_mime_magic_conf_t));
    if (conf == NULL)
    {
        return NULL;
    }

    conf->enable = NGX_CONF_UNSET;

    return conf;
}

static char *ngx_http_mime_magic_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_mime_magic_conf_t *prev = parent;
    ngx_http_mime_magic_conf_t *conf = child;

    ngx_conf_merge_value(conf->enable, prev->enable, 0);
    ngx_conf_merge_str_value(conf->magic_db, prev->magic_db, '\0');

    if (conf->enable)
    {
        magic_cookie = magic_open(MAGIC_MIME_TYPE);

        magic_load(magic_cookie, (char *)conf->magic_db.data);

        const char *err = magic_error(magic_cookie);
        if (err != NULL)
        {
            ngx_log_error(NGX_LOG_ERR, cf->log, 0, err);
            return NGX_CONF_ERROR;
        }
    }

    return NGX_CONF_OK;
}

static ngx_int_t
ngx_http_mime_magic_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_mime_magic_header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_mime_magic_body_filter;

    return NGX_OK;
}