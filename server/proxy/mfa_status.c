#include <winpr/synch.h>

#include "mfa_status.h"

void proxy_data_update_mfa_status(proxyData* pdata, MFA_STATUS status)
{
	EnterCriticalSection(&pdata->lock);
	pdata->mfaStatus = status;
	SetEvent(pdata->mfaEvent);
	LeaveCriticalSection(&pdata->lock);
}

void proxy_data_get_mfa_status(proxyData* pdata, MFA_STATUS* status)
{
	EnterCriticalSection(&pdata->lock);
	*status = pdata->mfaStatus;
	LeaveCriticalSection(&pdata->lock);
}