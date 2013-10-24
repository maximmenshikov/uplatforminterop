#include "stdafx.h"
#include "..\..\common\Account.hpp"
#include "..\..\common\pacmanclient.h"
#include "stringloader.h"
#include "tinyxml2.h"

using namespace tinyxml2;

#define DEBUGMODE 0

template<class T, std::size_t N> 
char(&countof(T(&)[N]))[N];

#define countof(arr) sizeof(countof(arr))

void BNSICreateNotification() {} 
void BNSIDeleteNotification() {} 
void BNSIGetNotificationByID() {} 
void BNSIGetNotificationByIndex() {} 
void BNSIGetNotificationCount() {} 
void BNSILaunchNotification() {} 
void BNSILoadErrorMessage() {} 
void BNSIUpdateNotification() {} 
void BeginManagingVibration() {} 
void CompleteTask() {} 
void DestroyTaskCallbacks() {} 
void DevicePropertiesGetFirmwareVersion() {} 
void DevicePropertiesGetHardwareVersion() {} 
void DevicePropertiesGetIMSI() {} 
void DevicePropertiesGetUniqueDeviceId() {} 
void GetCurrentProcessHandle() {} 
void GetTaskInputBuffer() {} 
void Initialize() {} 
void IsClsidApproved() {} 
void LNGetAppTaskInfo() {} 
void NativeGetNetworkType() {} 
void OnFullObscurityChanged() {} 
void RegisterAppCallbacks() {} 
void RegisterComDllInproc() {} 
void RegisterNetworkNotification() {} 
void RegisterTaskCallbacks() {} 
void RegistryWatchInitialize() {} 
void ResolveHostName() {} 
void SetHostErrorCode() {} 
void StartRegistryWatchCallback() {} 
void Stop() {} 
void StopManagingVibration() {} 
void StopRegistryWatchCallback() {} 
void UnInitialize() {} 
void UnregisterNetworkNotification() {} 
void Vibrate() {} 

typedef HRESULT (*GETPROP)(wchar_t *str, DWORD length);

HMODULE hMsPlatformInterop = NULL;

GETPROP GetManufacturer = NULL;
GETPROP GetModel = NULL;

bool triedLoading = false;

void Load()
{
	RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] Load(hLib = %X, triedLoading = %d)\r\n", hMsPlatformInterop, triedLoading));
	if (hMsPlatformInterop == NULL && triedLoading == false)
	{
		hMsPlatformInterop = LoadLibrary(L"msPlatformInterop.dll");
		GetManufacturer = (GETPROP)GetProcAddress(hMsPlatformInterop, L"DevicePropertiesGetManufacturer");
		GetModel = (GETPROP)GetProcAddress(hMsPlatformInterop, L"DevicePropertiesGetManufacturer");

		RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] Loaded: hLib = %X; %X %X\r\n", hMsPlatformInterop, 
						GetManufacturer, GetModel));

		triedLoading = true;
	}
}


BOOL GetProductID(wchar_t *account, wchar_t *productID, DWORD length)
{
	memset(productID, 0, length * sizeof(wchar_t));
	account = wcsrchr(account, L'X');
	if (account)
	{
		account++;
		if (wcslen(account) >= 76)
		{
			wchar_t s[3];
			s[2] = 0;
			for (int x = 0; x < 76; x += 2)
			{
				s[0] = account[0];
				s[1] = account[1];

				DWORD symbol = wcstoul(s, NULL, 16);
				productID[x / 2] = symbol;
				account += 2;
			}
			return TRUE;
		}
	}
	return FALSE;
	//S-1-5-112-0-0X80-0X7B42353338324442462D303932332D343139352D423638422D4639334237454537364645397D
}

BOOL GetAppManifest(wchar_t *account, wchar_t *installFolder, DWORD length)
{
	RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] GetAppManifest(%ls)++\n", account));
	if (account == NULL || installFolder == NULL)
		return FALSE;
	memset(installFolder, 0, sizeof(wchar_t) * length);

	wchar_t productID[72];
	memset(productID, 0, sizeof(productID));
	account = wcsrchr(account, L'X');

	if (account)
	{
		account++;
		if (wcslen(account) >= 76)
		{
			wchar_t s[3];
			s[2] = 0;
			account += 2;
			for (int x = 0; x < 72; x+=2)
			{
				s[0] = account[x];
				s[1] = account[x+1];

				DWORD symbol = wcstoul(s, NULL, 16);
				productID[x / 2] = symbol;
			}
			wcscpy_s(installFolder, length, L"\\Applications\\Install\\");
			wcscat_s(installFolder, length, productID);
			wcscat_s(installFolder, length, L"\\Install\\WMAppManifest.xml");
			return TRUE;
		}
	}
	return FALSE;
}

wchar_t *LoadAuthorFromXml(wchar_t *fileName)
{
	const char *result = NULL;
	FILE *file = _wfopen(fileName, L"rb");
	if (file)
	{
		XMLDocument doc;
		if (doc.LoadFile(file) == 0)
		{
			char *levels[] = {"Deployment", "App", "Author"};
			int levelCount = countof(levels);
			XMLElement* elem = doc.FirstChildElement(levels[0]);
			for (int i = 1; i < levelCount; i++)
			{
				if (i == (levelCount - 1))
				{
					result = elem->Attribute(levels[i]);
				}
				else
				{
					elem = elem->FirstChildElement(levels[i]);
					if (!elem)
						break;
				}
			}
		}
		fclose(file);
	}
	if (result)
	{
		wchar_t *str = new wchar_t[strlen(result) + 1];
		mbstowcs(str, result, strlen(result) + 1);
		return str;
	}
	return NULL;
}

wchar_t *GetAuthor()
{
	wchar_t *authorLocalized = NULL;
	RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] GetAuthor++\r\n"));
	ACCTID account = Security::Account::GetAccount(CURRENTPROCESS);
	wchar_t accountName[200];
	if (Security::Account::GetAccountName(account, accountName, 200) == TRUE)
	{
		RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] AccountName = \"%ls\"\r\n", accountName));
		wchar_t manifestPath[500];
		if (GetAppManifest(accountName, manifestPath, countof(manifestPath)) == TRUE)
		{
			RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] ManifestPath = \"%ls\"\r\n", manifestPath));
			wchar_t *xmlAuthor = LoadAuthorFromXml(manifestPath);
			if (xmlAuthor)
			{
				RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] XmlAuthor = \"%ls\"\r\n", xmlAuthor));
				wchar_t *authorNew = NULL;
				__try
				{
					RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] Entering GetLocalized\r\n"));
					authorNew = GetLocalizedString(xmlAuthor);
					//delete[] xmlAuthor;
					authorLocalized = authorNew;
				}
				__except (1)
				{
					authorLocalized = NULL;
				}
				RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] authorLocalized = %ls\r\n", authorLocalized));
				if (authorLocalized == NULL)
				{
					authorLocalized = SysAllocString(xmlAuthor);
				}
				RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] delete[] xmlAuthor\r\n"));
				delete[] xmlAuthor;
				RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] delete[] xmlAuthor SUCCESS\r\n"));
				return authorLocalized;
			}
		}
	}
	return NULL;
}

typedef struct
{
	wchar_t *author;
	wchar_t *targetManufacturer;
	wchar_t *targetModel;
}MODELINFO;

#define MODEL_COUNT 1

MODELINFO models[] = {
	{L"nokia", L"NOKIA", L"Lumia 900"}		
};

const wchar_t* wcsistr(const wchar_t *str, const wchar_t *pattern)
{
	size_t len;
	for (len = wcslen(pattern); *str != '\0' && wcslen(str) >= len; str++)
	{
		if(_wcsnicmp(str, pattern, len) == 0)
			return str;
	}
	return NULL;
}

MODELINFO* ManufacturerFindMatch(wchar_t *author)
{
	RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] FindMatch (%ls)\r\n", author));
	if (author == NULL)
		return NULL;
	for (int i = 0; i < MODEL_COUNT; ++i)
	{
		MODELINFO *model = &models[i];
		RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] Comparing \"%ls\" with \"%ls\"\r\n", author, model->author));
		if (wcsistr(author, model->author))
		{
			RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] Matched! \"%ls\" with \"%ls\"\r\n", author, model->author));
			//printf("ManufacturerTest(%ls) - MATCHED with %ls %ls\r\n", author, model->targetManufacturer, model->targetModel);
			return model;
		}
	}
	RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] FindMatch - No matches!!!\r\n"));
	//printf("ManufacturerTest(%ls) - no matches\r\n", author);
	return NULL;
}

HRESULT DevicePropertiesGetManufacturer(wchar_t *str, DWORD len) 
{
	RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] GetManufacturer++\r\n"));
	Load();
	memset(str, 0, len * sizeof(wchar_t));
	bool useStock = true;
	if (str && len)
	{
		wchar_t *author = GetAuthor();
		if (author)
		{
			MODELINFO *model = ManufacturerFindMatch(author);
			if (model)
			{
				RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] Using manufacturer \"%ls\"\r\n", model->targetManufacturer));
				wcscpy_s(str, len, model->targetManufacturer);
				useStock = false;
			}
			SysFreeString(author);
		}
	}
	if (useStock)
	{
		RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] Using stock manufacturer\r\n"));
		return GetManufacturer(str, len);
	}
	return S_OK;
} 

HRESULT DevicePropertiesGetModel(wchar_t *str, DWORD len) 
{
	RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] GetModel++\r\n"));
	Load();
	memset(str, 0, len * sizeof(wchar_t));
	bool useStock = true;
	if (str && len)
	{
		wchar_t *author = GetAuthor();
		if (author)
		{
			MODELINFO *model = ManufacturerFindMatch(author);
			if (model)
			{
				RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] Using model \"%ls\"\r\n", model->targetModel));
				wcscpy_s(str, len, model->targetModel);
				useStock = false;
			}
			SysFreeString(author);
		}
	}
	if (useStock)
	{
		RETAILMSG(DEBUGMODE, (L"[UPLATFORMINTEROP] Using stock model\r\n"));
		return GetModel(str, len);
	}
	return S_OK;
} 


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

