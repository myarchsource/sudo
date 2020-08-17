/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (c) 2009-2015, 2019-2020 Todd C. Miller <Todd.Miller@sudo.ws>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This is an open source non-commercial project. Dear PVS-Studio, please check it.
 * PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
 */

#include <config.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sudoers.h"

#ifdef HAVE_BSM_AUDIT
# include "bsm_audit.h"
#endif
#ifdef HAVE_LINUX_AUDIT
# include "linux_audit.h"
#endif
#ifdef HAVE_SOLARIS_AUDIT
# include "solaris_audit.h"
#endif

char *audit_msg = NULL;

static int
audit_success(char *const argv[])
{
    int rc = 0;
    debug_decl(audit_success, SUDOERS_DEBUG_AUDIT);

    if (argv != NULL) {
#ifdef HAVE_BSM_AUDIT
	if (bsm_audit_success(argv) == -1)
	    rc = -1;
#endif
#ifdef HAVE_LINUX_AUDIT
	if (linux_audit_command(argv, 1) == -1)
	    rc = -1;
#endif
#ifdef HAVE_SOLARIS_AUDIT
	if (solaris_audit_success(argv) == -1)
	    rc = -1;
#endif
    }

    debug_return_int(rc);
}

static int
audit_failure_int(char *const argv[], const char *message)
{
    int ret = 0;
    debug_decl(audit_failure_int, SUDOERS_DEBUG_AUDIT);

#if defined(HAVE_BSM_AUDIT) || defined(HAVE_LINUX_AUDIT)
    if (def_log_denied && argv != NULL) {
#ifdef HAVE_BSM_AUDIT
	if (bsm_audit_failure(argv, message) == -1)
	    ret = -1;
#endif
#ifdef HAVE_LINUX_AUDIT
	if (linux_audit_command(argv, 0) == -1)
	    ret = -1;
#endif
#ifdef HAVE_SOLARIS_AUDIT
	if (solaris_audit_failure(argv, message) == -1)
	    ret = -1;
#endif
    }
#endif /* HAVE_BSM_AUDIT || HAVE_LINUX_AUDIT */

    debug_return_int(ret);
}

int
audit_failure(char *const argv[], char const *const fmt, ...)
{
    int oldlocale, ret;
    char *message;
    va_list ap;
    debug_decl(audit_failure, SUDOERS_DEBUG_AUDIT);

    /* Audit messages should be in the sudoers locale. */
    sudoers_setlocale(SUDOERS_LOCALE_SUDOERS, &oldlocale);

    va_start(ap, fmt);
    if ((ret = vasprintf(&message, _(fmt), ap)) == -1)
	sudo_warnx(U_("%s: %s"), __func__, U_("unable to allocate memory"));
    va_end(ap);

    if (ret != -1) {
	/* Set audit_msg for audit plugins.  */
	free(audit_msg);
	audit_msg = message;

	ret = audit_failure_int(argv, audit_msg);
    }

    sudoers_setlocale(oldlocale, NULL);

    debug_return_int(ret);
}

static int
sudoers_audit_open(unsigned int version, sudo_conv_t conversation,
    sudo_printf_t plugin_printf, char * const settings[],
    char * const user_info[], int submit_optind, char * const submit_argv[],
    char * const submit_envp[], char * const plugin_options[],
    const char **errstr)
{
    struct sudo_conf_debug_file_list debug_files = TAILQ_HEAD_INITIALIZER(debug_files);
    struct sudoers_open_info info;
    const char *cp, *plugin_path = NULL;
    char * const *cur;
    int ret;
    debug_decl(sudoers_audit_open, SUDOERS_DEBUG_PLUGIN);

    sudo_conv = conversation;
    sudo_printf = plugin_printf;

    bindtextdomain("sudoers", LOCALEDIR);

    /* Initialize the debug subsystem.  */
    for (cur = settings; (cp = *cur) != NULL; cur++) {
	if (strncmp(cp, "debug_flags=", sizeof("debug_flags=") - 1) == 0) {
	    cp += sizeof("debug_flags=") - 1;
	    if (!sudoers_debug_parse_flags(&debug_files, cp))
		debug_return_int(-1);
	    continue;
	}
	if (strncmp(cp, "plugin_path=", sizeof("plugin_path=") - 1) == 0) {
	    plugin_path = cp + sizeof("plugin_path=") - 1;
	    continue;
	}
    }
    if (!sudoers_debug_register(plugin_path, &debug_files))
	debug_return_int(-1);

    /* Call the sudoers init function. */
    info.settings = settings;
    info.user_info = user_info;
    info.plugin_args = plugin_options;
    ret = sudoers_init(&info, submit_envp);

    /* The audit functions set audit_msg on failure. */
    if (ret != 1 && audit_msg != NULL)
	*errstr = audit_msg;

    debug_return_int(ret);
}

static int
sudoers_audit_accept(const char *plugin_name, unsigned int plugin_type,
    char * const command_info[], char * const run_argv[],
    char * const run_envp[], const char **errstr)
{
    int ret = true;
    debug_decl(sudoers_audit_accept, SUDOERS_DEBUG_PLUGIN);

    /* Only log the accept event from the sudo front-end */
    if (plugin_type != SUDO_FRONT_END)
	debug_return_int(true);

    if (def_log_allowed) {
	if (audit_success(run_argv) != 0 && !def_ignore_audit_errors)
	    ret = false;

	if (!log_allowed(VALIDATE_SUCCESS) && !def_ignore_logfile_errors)
	    ret = false;
    }

    debug_return_int(ret);
}

static int
sudoers_audit_reject(const char *plugin_name, unsigned int plugin_type,
    const char *message, char * const command_info[], const char **errstr)
{
    int ret = true;
    char *logline;
    debug_decl(sudoers_audit_reject, SUDOERS_DEBUG_PLUGIN);

    /* Skip reject events that sudoers generated itself. */
    if (strncmp(plugin_name, "sudoers_", 8) == 0)
	debug_return_int(true);

    if (!def_log_denied)
	debug_return_int(true);

    if (audit_failure_int(NewArgv, message) != 0) {
	if (!def_ignore_audit_errors)
	    ret = false;
    }

    logline = new_logline(message, NULL);
    if (logline == NULL) {
	sudo_warnx(U_("%s: %s"), __func__, U_("unable to allocate memory"));
	ret = false;
	goto done;
    }
    if (def_syslog) {
	if (!do_syslog(def_syslog_badpri, logline)) {
	    if (!def_ignore_logfile_errors)
		ret = false;
	}
    }
    if (def_logfile) {
	if (!do_logfile(logline)) {
	    if (!def_ignore_logfile_errors)
		ret = false;
	}
    }
    free(logline);

done:
    debug_return_int(ret);
}

static int
sudoers_audit_error(const char *plugin_name, unsigned int plugin_type,
    const char *message, char * const command_info[], const char **errstr)
{
    int ret = true;
    debug_decl(sudoers_audit_error, SUDOERS_DEBUG_PLUGIN);

    /* Skip error events that sudoers generated itself. */
    if (strncmp(plugin_name, "sudoers_", 8) == 0)
	debug_return_int(true);

    if (audit_failure_int(NewArgv, message) != 0) {
	if (!def_ignore_audit_errors)
	    ret = false;
    }
    if (def_syslog) {
	if (!do_syslog(def_syslog_badpri, message)) {
	    if (!def_ignore_logfile_errors)
		ret = false;
	}
    }
    if (def_logfile) {
	if (!do_logfile(message)) {
	    if (!def_ignore_logfile_errors)
		ret = false;
	}
    }

    debug_return_int(ret);
}

static int
sudoers_audit_version(int verbose)
{
    debug_decl(sudoers_audit_version, SUDOERS_DEBUG_PLUGIN);

    sudo_printf(SUDO_CONV_INFO_MSG, "Sudoers audit plugin version %s\n",
        PACKAGE_VERSION);

    debug_return_int(true);
}

__dso_public struct audit_plugin sudoers_audit = {
    SUDO_AUDIT_PLUGIN,
    SUDO_API_VERSION,
    sudoers_audit_open,
    NULL, /* audit_close */
    sudoers_audit_accept,
    sudoers_audit_reject,
    sudoers_audit_error,
    sudoers_audit_version,
    NULL, /* register_hooks */
    NULL /* deregister_hooks */
};
