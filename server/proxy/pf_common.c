#include "pf_common.h"

BOOL pf_common_connection_aborted_by_peer(proxyContext* context)
{
	return WaitForSingleObject(context->connectionClosed, 0) == WAIT_OBJECT_0;
}

void pf_common_copy_settings(rdpSettings* dst, rdpSettings* src)
{
	/* Client/server CORE options */
	dst->RdpVersion = src->RdpVersion;
	dst->DesktopWidth = src->DesktopWidth;
	dst->DesktopHeight = src->DesktopHeight;
	dst->ColorDepth = src->ColorDepth;
	dst->ConnectionType = src->ConnectionType;
	dst->ClientBuild = src->ClientBuild;
	dst->ClientHostname = _strdup(src->ClientHostname);
	dst->ClientProductId = _strdup(src->ClientProductId);
	dst->EarlyCapabilityFlags = src->EarlyCapabilityFlags;
	dst->NetworkAutoDetect = src->NetworkAutoDetect;
	dst->SupportAsymetricKeys = src->SupportAsymetricKeys;
	dst->SupportErrorInfoPdu = src->SupportErrorInfoPdu;
	dst->SupportStatusInfoPdu = src->SupportStatusInfoPdu;
	dst->SupportMonitorLayoutPdu = src->SupportMonitorLayoutPdu;
	dst->SupportGraphicsPipeline = src->SupportGraphicsPipeline;
	dst->SupportDynamicTimeZone = src->SupportDynamicTimeZone;
	dst->SupportHeartbeatPdu = src->SupportHeartbeatPdu;
	dst->DesktopPhysicalWidth = src->DesktopPhysicalWidth;
	dst->DesktopPhysicalHeight = src->DesktopPhysicalHeight;
	dst->DesktopOrientation = src->DesktopOrientation;
	dst->DesktopScaleFactor = src->DesktopScaleFactor;
	dst->DeviceScaleFactor = src->DeviceScaleFactor;
	/* client info */
	dst->AutoLogonEnabled = src->AutoLogonEnabled;
	dst->CompressionEnabled = src->CompressionEnabled;
	dst->DisableCtrlAltDel = src->DisableCtrlAltDel;
	dst->EnableWindowsKey = src->EnableWindowsKey;
	dst->MaximizeShell = src->MaximizeShell;
	dst->LogonNotify = src->LogonNotify;
	dst->LogonErrors = src->LogonErrors;
	dst->MouseAttached = src->MouseAttached;
	dst->MouseHasWheel = src->MouseHasWheel;
	dst->RemoteConsoleAudio = src->RemoteConsoleAudio;
	dst->AudioPlayback = src->AudioPlayback;
	dst->AudioCapture = src->AudioCapture;
	dst->VideoDisable = src->VideoDisable;
	dst->PasswordIsSmartcardPin = src->PasswordIsSmartcardPin;
	dst->UsingSavedCredentials = src->UsingSavedCredentials;
	dst->ForceEncryptedCsPdu = src->ForceEncryptedCsPdu;
	dst->HiDefRemoteApp = src->HiDefRemoteApp;
	dst->CompressionLevel = src->CompressionLevel;
	dst->PerformanceFlags = src->PerformanceFlags;
	dst->AllowFontSmoothing = src->AllowFontSmoothing;
	dst->DisableWallpaper = src->DisableWallpaper;
	dst->DisableFullWindowDrag = src->DisableFullWindowDrag;
	dst->DisableMenuAnims = src->DisableMenuAnims;
	dst->DisableThemes = src->DisableThemes;
	dst->DisableCursorShadow = src->DisableCursorShadow;
	dst->DisableCursorBlinking = src->DisableCursorBlinking;
	dst->AllowDesktopComposition = src->AllowDesktopComposition;
	dst->DisableThemes = src->DisableThemes;
	/* Remote App */
	dst->RemoteApplicationMode = src->RemoteApplicationMode;
	dst->RemoteApplicationName = src->RemoteApplicationName;
	dst->RemoteApplicationIcon = src->RemoteApplicationIcon;
	dst->RemoteApplicationProgram = src->RemoteApplicationProgram;
	dst->RemoteApplicationFile = src->RemoteApplicationFile;
	dst->RemoteApplicationGuid = src->RemoteApplicationGuid;
	dst->RemoteApplicationCmdLine = src->RemoteApplicationCmdLine;
	dst->RemoteApplicationExpandCmdLine = src->RemoteApplicationExpandCmdLine;
	dst->RemoteApplicationExpandWorkingDir = src->RemoteApplicationExpandWorkingDir;
	dst->DisableRemoteAppCapsCheck = src->DisableRemoteAppCapsCheck;
	dst->RemoteAppNumIconCaches = src->RemoteAppNumIconCaches;
	dst->RemoteAppNumIconCacheEntries = src->RemoteAppNumIconCacheEntries;
	dst->RemoteAppLanguageBarSupported = src->RemoteAppLanguageBarSupported;
	dst->RemoteWndSupportLevel = src->RemoteWndSupportLevel;
	/* GFX */
	dst->GfxThinClient = src->GfxThinClient;
	dst->GfxSmallCache = src->GfxSmallCache;
	dst->GfxProgressive = src->GfxProgressive;
	dst->GfxProgressiveV2 = src->GfxProgressiveV2;
	dst->GfxH264 = src->GfxH264;
	dst->GfxAVC444 = src->GfxAVC444;
	dst->GfxSendQoeAck = src->GfxSendQoeAck;
	dst->GfxAVC444v2 = src->GfxAVC444v2;
	dst->SurfaceCommandsEnabled = src->SurfaceCommandsEnabled;
	dst->FrameMarkerCommandEnabled = src->FrameMarkerCommandEnabled;
	dst->SurfaceFrameMarkerEnabled = src->SurfaceFrameMarkerEnabled;
	dst->RefreshRect = src->RefreshRect;
	dst->SuppressOutput = src->SuppressOutput;
	dst->FastPathOutput = src->FastPathOutput;
	dst->SaltedChecksum = src->SaltedChecksum;
	dst->LongCredentialsSupported = src->LongCredentialsSupported;
	dst->NoBitmapCompressionHeader = src->NoBitmapCompressionHeader;
	dst->BitmapCompressionDisabled = src->BitmapCompressionDisabled;
	dst->DesktopResize = src->DesktopResize;
	dst->DrawAllowDynamicColorFidelity = src->DrawAllowDynamicColorFidelity;
	dst->DrawAllowColorSubsampling = src->DrawAllowColorSubsampling;
	dst->DrawAllowSkipAlpha = src->DrawAllowSkipAlpha;
	dst->OrderSupport = src->OrderSupport;
	dst->BitmapCacheV3Enabled = src->BitmapCacheV3Enabled;
	dst->AltSecFrameMarkerSupport = src->AltSecFrameMarkerSupport;
	dst->AllowUnanouncedOrdersFromServer = src->AllowUnanouncedOrdersFromServer;
	dst->BitmapCacheEnabled = src->BitmapCacheEnabled;
	dst->BitmapCacheVersion = src->BitmapCacheVersion;
	dst->AllowCacheWaitingList = src->AllowCacheWaitingList;
	dst->BitmapCachePersistEnabled = src->BitmapCachePersistEnabled;
	dst->BitmapCacheV2NumCells = src->BitmapCacheV2NumCells;
	dst->BitmapCacheV2CellInfo = src->BitmapCacheV2CellInfo;
	dst->ColorPointerFlag = src->ColorPointerFlag;
	dst->PointerCacheSize = src->PointerCacheSize;
}