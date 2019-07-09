#ifndef FREERDP_SERVER_PROXY_MFA_STATUS_H
#define FREERDP_SERVER_PROXY_MFA_STATUS_H

#include "pf_context.h"

typedef struct proxy_data proxyData;

typedef enum
{
	MFA_STATUS_AUTH_SUCCESS = 1,
	MFA_STATUS_AUTH_FAIL,
	MFA_STATUS_TOKEN_EXPIRED
} MFA_STATUS;

void proxy_data_update_mfa_status(proxyData* pdata, MFA_STATUS status);
void proxy_data_get_mfa_status(proxyData* pdata, MFA_STATUS* status);

#endif /* FREERDP_SERVER_PROXY_MFA_STATUS_H */
