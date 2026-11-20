/* Helper for user_config.c: a CXTR_USER_CONFIG header. */
#ifndef USER_CONFIG_H
#define USER_CONFIG_H

/* The version is already defined when the config is included. */
#if CXTR_VERSION < 20261100L
#  error "CXTR_VERSION must be visible to the user config"
#endif

/* Overrides: checks stay on despite NDEBUG, failures go to a project
 * handler, and CXTR_MUST_USE is disabled. */
#define CXTR_CHECK_ASSUMPTIONS 1
void project_contract_failed(const char *text);
#define CXTR_CONTRACT_FAILED(text) project_contract_failed(text)
#define CXTR_MUST_USE

#endif
