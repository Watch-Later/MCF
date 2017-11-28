// 这个文件是 MCF 的一部分。
// 有关具体授权说明，请参阅 MCFLicense.txt。
// Copyleft 2013 - 2017, LH_Mouse. All wrongs reserved.

#include "variable_map.h"
#include "heap.h"
#include "last_error.h"
#include "endian.h"
#include "sha256.h"

static inline bool ValidateKey(size_t *restrict puSizeOfKey, const wchar_t *restrict pwszKey){
	// Get the array index of the first character that is not acceptable.
	size_t uIndex = wcsspn(pwszKey, L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_");
	// If `uIndex` does not designate the null terminator, the key is invalid.
	if(pwszKey[uIndex] != 0){
		return false;
	}
	// If it does, `uIndex` will equal the length of the key. The key is invalid if has a length of zero.
	if(uIndex == 0){
		return false;
	}
	*puSizeOfKey = uIndex * sizeof(wchar_t);
	return true;
}

bool MCFBUILD_VariableMapIsKeyValid(const wchar_t *pwszKey){
	size_t uSizeOfKey;
	if(!ValidateKey(&uSizeOfKey, pwszKey)){
		MCFBUILD_SetLastError(ERROR_INVALID_NAME);
		return false;
	}
	return true;
}

typedef struct tagMapElement {
	size_t uSizeWhole;
	size_t uOffsetToValue;
	wchar_t awcString[];
} MapElement;

void MCFBUILD_VariableMapInitialize(MCFBUILD_VariableMap *pMap){
	pMap->pbyStorage = 0;
	pMap->uCapacity = 0;
	pMap->uOffsetEnd = 0;
}
void MCFBUILD_VariableMapUninitialize(MCFBUILD_VariableMap *pMap){
	MCFBUILD_HeapFree(pMap->pbyStorage);
}

void MCFBUILD_VariableMapClear(MCFBUILD_VariableMap *pMap){
	pMap->uOffsetEnd = 0;
}
bool MCFBUILD_VariableMapGet(const wchar_t **restrict ppwszValue, size_t *restrict puLength, const MCFBUILD_VariableMap *restrict pMap, const wchar_t *pwszKey){
	size_t uSizeOfKey;
	if(!ValidateKey(&uSizeOfKey, pwszKey)){
		MCFBUILD_SetLastError(ERROR_INVALID_NAME);
		return false;
	}
	/*-----------------------------------------------------------*\
	|         /------------- MapElement                           |
	|         |/------------ Beginning of key                     |
	|         || /---------- Null terminator of key               |
	|         || |/--------- Beginning of padding                 |
	|         || || /------- Beginning of value                   |
	|         || || |   /--- Null terminator of value             |
	|         || || |   |                                         |
	| BEFORE  *##_??====_*#_==_*##_=_*###_?====_*####_???==_      |
	|         ^storage                ^key                  ^end  |
	| AFTER   *##_??====_*#_==_*##_=_*###_?====_*####_???==_      |
	|         ^storage                ^key ^string                |
	|                                      \__/length             |
	\*-----------------------------------------------------------*/
	const unsigned char *pbyStorage = pMap->pbyStorage;
	const MapElement *pElement;
	size_t uCapacityTaken;
	size_t uOffsetCursor = 0;
	for(;;){
		pElement = (const void *)(pbyStorage + uOffsetCursor);
		if(uOffsetCursor == pMap->uOffsetEnd){
			MCFBUILD_SetLastError(ERROR_NOT_FOUND);
			return false;
		}
		uCapacityTaken = sizeof(MapElement) + pElement->uSizeWhole;
		if(wcscmp(pElement->awcString, pwszKey) == 0){
			break;
		}
		uOffsetCursor += uCapacityTaken;
	}
	size_t uSizeOfValue = pElement->uSizeWhole - pElement->uOffsetToValue - sizeof(wchar_t);
	*ppwszValue = (const void *)((const unsigned char *)(pElement->awcString) + pElement->uOffsetToValue);
	*puLength = uSizeOfValue / sizeof(wchar_t);
	return true;
}
bool MCFBUILD_VariableMapSet(MCFBUILD_VariableMap *restrict pMap, const wchar_t *restrict pwszKey, const wchar_t *restrict pwcValue, size_t uLength){
	size_t uSizeOfKey;
	if(!ValidateKey(&uSizeOfKey, pwszKey)){
		MCFBUILD_SetLastError(ERROR_INVALID_NAME);
		return false;
	}
	/*-----------------------------------------------------------*\
	|         /------------- MapElement                           |
	|         |/------------ Beginning of key                     |
	|         || /---------- Null terminator of key               |
	|         || |/--------- Beginning of padding                 |
	|         || || /------- Beginning of value                   |
	|         || || |   /--- Null terminator of value             |
	|         || || |   |                                         |
	| BEFORE  *##_??====_*#_==_*##_=_*###_?====_*####_???==_      |
	|         ^storage                ^key                  ^end  |
	| AFTER   *##_??====_*#_==_*##_=_*###_?====_*####_???==_      |
	|         ^storage                ^key ^string                |
	|                                      \__/length             |
	\*-----------------------------------------------------------*/
	unsigned char *pbyStorage = pMap->pbyStorage;
	MapElement *pElement;
	size_t uCapacityTaken;
	size_t uOffsetCursor = 0;
	for(;;){
		pElement = (void *)(pbyStorage + uOffsetCursor);
		if(uOffsetCursor == pMap->uOffsetEnd){
			uCapacityTaken = 0;
			break;
		}
		uCapacityTaken = sizeof(MapElement) + pElement->uSizeWhole;
		if(wcscmp(pElement->awcString, pwszKey) == 0){
			break;
		}
		uOffsetCursor += uCapacityTaken;
	}
	size_t uSizeOfValue = uLength * sizeof(wchar_t);
	size_t uSizeWhole = uSizeOfKey + sizeof(wchar_t) + uSizeOfValue + sizeof(wchar_t);
	unsigned char bySizePadded = -uSizeWhole % 8;
	uSizeWhole += bySizePadded;
	ptrdiff_t nDistanceToMove = (ptrdiff_t)(sizeof(MapElement) + uSizeWhole) - (ptrdiff_t)uCapacityTaken;
	if(nDistanceToMove > 0){
		size_t uMinimumSizeToReserve;
		if(__builtin_add_overflow(pMap->uOffsetEnd, (size_t)nDistanceToMove, &uMinimumSizeToReserve)){
			MCFBUILD_SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			return false;
		}
		size_t uCapacity = pMap->uCapacity;
		if(uCapacity < uMinimumSizeToReserve){
			uCapacity += uCapacity / 2;
			uCapacity += 0x0F;
			uCapacity &= (size_t)-0x10;
			uCapacity |= uMinimumSizeToReserve;
			uCapacity |= 0x400;
			pbyStorage = MCFBUILD_HeapRealloc(pbyStorage, uCapacity);
			if(!pbyStorage){
				return false;
			}
			pMap->pbyStorage = pbyStorage;
			pMap->uCapacity = uCapacity;
			pElement = (void *)(pbyStorage + uOffsetCursor);
		}
	}
	if(nDistanceToMove != 0){
		memmove(pbyStorage + uOffsetCursor + uCapacityTaken + nDistanceToMove, pbyStorage + uOffsetCursor + uCapacityTaken, pMap->uOffsetEnd - uOffsetCursor - uCapacityTaken);
		pMap->uOffsetEnd += (size_t)nDistanceToMove;
	}
	size_t uOffsetToValue = uSizeWhole - uSizeOfValue - sizeof(wchar_t);
	pElement->uSizeWhole = uSizeWhole;
	pElement->uOffsetToValue = uOffsetToValue;
	if(uCapacityTaken == 0){
		// Copy the key only if there wasn't one. The key will always be null-terminated as well as `pwszKey`.
		memcpy(pElement->awcString, pwszKey, uSizeOfKey + sizeof(wchar_t));
	}
	memcpy((unsigned char *)(pElement->awcString) + uOffsetToValue, pwcValue, uSizeOfValue);
	*(wchar_t *)((unsigned char *)(pElement->awcString) + uOffsetToValue + uSizeOfValue) = 0;
	return true;
}
bool MCFBUILD_VariableMapSetNullTerminated(MCFBUILD_VariableMap *restrict pMap, const wchar_t *restrict pwszKey, const wchar_t *restrict pwszValue){
	return MCFBUILD_VariableMapSet(pMap, pwszKey, pwszValue, wcslen(pwszValue));
}
bool MCFBUILD_VariableMapUnset(MCFBUILD_VariableMap *restrict pMap, const wchar_t *restrict pwszKey){
	size_t uSizeOfKey;
	if(!ValidateKey(&uSizeOfKey, pwszKey)){
		MCFBUILD_SetLastError(ERROR_INVALID_NAME);
		return false;
	}
	/*-----------------------------------------------------------*\
	|         /------------- MapElement                           |
	|         |/------------ Beginning of key                     |
	|         || /---------- Null terminator of key               |
	|         || |/--------- Beginning of padding                 |
	|         || || /------- Beginning of value                   |
	|         || || |   /--- Null terminator of value             |
	|         || || |   |                                         |
	| BEFORE  *##_??====_*#_==_*##_=_*###_?====_*####_???==_      |
	|         ^storage                ^key                  ^end  |
	| AFTER   *##_??====_*#_==_*##_=_*####_???==_                 |
	|         ^storage                           ^end             |
	\*-----------------------------------------------------------*/
	unsigned char *pbyStorage = pMap->pbyStorage;
	const MapElement *pElement;
	size_t uCapacityTaken;
	size_t uOffsetCursor = 0;
	for(;;){
		pElement = (const void *)(pbyStorage + uOffsetCursor);
		if(uOffsetCursor == pMap->uOffsetEnd){
			MCFBUILD_SetLastError(ERROR_NOT_FOUND);
			return false;
		}
		uCapacityTaken = sizeof(MapElement) + pElement->uSizeWhole;
		if(wcscmp(pElement->awcString, pwszKey) == 0){
			break;
		}
		uOffsetCursor += uCapacityTaken;
	}
	memmove(pbyStorage + uOffsetCursor, pbyStorage + uOffsetCursor + uCapacityTaken, pMap->uOffsetEnd - uOffsetCursor - uCapacityTaken);
	pMap->uOffsetEnd -= uCapacityTaken;
	return true;
}

bool MCFBUILD_VariableMapEnumerate(const wchar_t **restrict ppwszKey, const wchar_t **restrict ppwszValue, size_t *restrict puLength, MCFBUILD_VariableMapEnumerationCookie *restrict pCookie, const MCFBUILD_VariableMap *restrict pMap){
	/*-----------------------------------------------------------*\
	|         /------------- MapElement                           |
	|         |/------------ Beginning of key                     |
	|         || /---------- Null terminator of key               |
	|         || |/--------- Beginning of padding                 |
	|         || || /------- Beginning of value                   |
	|         || || |   /--- Null terminator of value             |
	|         || || |   |                                         |
	| BEFORE  *##_??====_*#_==_*##_=_*###_?====_*####_???==_      |
	|         ^storage                ^key                  ^end  |
	| AFTER   *##_??====_*#_==_*##_=_*###_?====_*####_???==_      |
	|         ^storage                ^key ^string                |
	|                                      \__/length             |
	\*-----------------------------------------------------------*/
	const unsigned char *pbyStorage = pMap->pbyStorage;
	const MapElement *pElement;
	size_t uCapacityTaken;
	size_t uOffsetCursor = pCookie->uOffsetNext;
	if(uOffsetCursor == pMap->uOffsetEnd){
		MCFBUILD_SetLastError(ERROR_NO_MORE_ITEMS);
		return false;
	}
	if(uOffsetCursor > pMap->uOffsetEnd){
		MCFBUILD_SetLastError(ERROR_INVALID_PARAMETER);
		return false;
	}
	pElement = (const void *)(pbyStorage + uOffsetCursor);
	uCapacityTaken = sizeof(MapElement) + pElement->uSizeWhole;
	*ppwszKey = pElement->awcString;
	size_t uSizeOfValue = pElement->uSizeWhole - pElement->uOffsetToValue - sizeof(wchar_t);
	*ppwszValue = (const void *)((const unsigned char *)(pElement->awcString) + pElement->uOffsetToValue);
	*puLength = uSizeOfValue / sizeof(wchar_t);
	uOffsetCursor += uCapacityTaken;
	pCookie->uOffsetNext = uOffsetCursor;
	return true;
}

typedef struct tagSerializedElement {
	uint64_t u64SizeWholeSerialized;
	uint64_t u64OffsetToValueSerialized;
	wchar_t awcString[];
} SerializedElement;

typedef struct tagSerializedHeader {
	uint8_t au8Checksum[32];
	unsigned char abyPayload[];
} SerializedHeader;

// This is the salt used to create checksums. The null terminator is part of the salt.
static const char kMagic[] = "MCFBUILD_VariableMap:2017-11-27";

bool MCFBUILD_VariableMapSerialize(void **restrict ppData, size_t *restrict puSize, const MCFBUILD_VariableMap *restrict pMap){
	// Estimate the upper bound of number of bytes to allocate.
	size_t uSizeToAlloc = sizeof(SerializedHeader);
	// Iterate from beginning to end, accumulating number of bytes for each element on the way.
	unsigned char *pbyStorage = pMap->pbyStorage;
	const MapElement *pElement;
	size_t uCapacityTaken;
	size_t uOffsetCursor = 0;
	while(uOffsetCursor != pMap->uOffsetEnd){
		pElement = (const void *)(pbyStorage + uOffsetCursor);
		uCapacityTaken = sizeof(MapElement) + pElement->uSizeWhole;
		// The key is serialized with its null terminator. The value is not.
		size_t uSizeOfKey = wcslen(pElement->awcString) * sizeof(wchar_t);
		size_t uSizeOfValue = pElement->uSizeWhole - pElement->uOffsetToValue - sizeof(wchar_t);
		size_t uSizeWhole = uSizeOfKey + uSizeOfValue;
		unsigned char bySizePadded = -uSizeWhole % 8;
		uSizeWhole += bySizePadded;
		uSizeToAlloc += sizeof(SerializedElement) + uSizeWhole;
		// Scan the next element.
		uOffsetCursor += uCapacityTaken;
	}
	// Allocate the buffer now.
	SerializedHeader *pHeader = MCFBUILD_HeapAlloc(uSizeToAlloc);
	if(!pHeader){
		return false;
	}
	// Populate the buffer with something predictable to ensure padding bytes have fixed values.
	memset(pHeader, -1, uSizeToAlloc);
	// Copy strings from beginning to end.
	unsigned char *pbyWrite = pHeader->abyPayload;
	uOffsetCursor = 0;
	while(uOffsetCursor != pMap->uOffsetEnd){
		pElement = (const void *)(pbyStorage + uOffsetCursor);
		uCapacityTaken = sizeof(MapElement) + pElement->uSizeWhole;
		// The key is serialized with its null terminator. The value is not.
		size_t uSizeOfKey = wcslen(pElement->awcString) * sizeof(wchar_t);
		size_t uSizeOfValue = pElement->uSizeWhole - pElement->uOffsetToValue - sizeof(wchar_t);
		size_t uSizeWhole = uSizeOfKey + uSizeOfValue;
		unsigned char bySizePadded = -uSizeWhole % 8;
		uSizeWhole += bySizePadded;
		// This is tricky. Acknowledging that `uSizeWhole` will be aligned onto an 8-byte boundary,
		// we add `bySizePadded` to it, preserving `bySizePadded` in its three LSBs intactly, with the rest being
		// `uSizeWhole` which can be fetched by bitwise and'ing the three LSBs away.
		SerializedElement *pSerialized = (void *)pbyWrite;
		MCFBUILD_store_be_uint64(&(pSerialized->u64SizeWholeSerialized), uSizeWhole + bySizePadded);
		MCFBUILD_store_be_uint64(&(pSerialized->u64OffsetToValueSerialized), uSizeWhole - uSizeOfValue);
		// Store the key without the null terminator.
		const wchar_t *pwcReadBase = pElement->awcString;
		wchar_t *pwcWriteBase = pSerialized->awcString;
		for(size_t uIndex = 0; uIndex < uSizeOfKey / sizeof(wchar_t); ++uIndex){
			MCFBUILD_move_be_uint16(pwcWriteBase + uIndex, pwcReadBase + uIndex);
		}
		// Store the value without the null terminator.
		pwcReadBase = pElement->awcString + pElement->uOffsetToValue / sizeof(wchar_t);
		pwcWriteBase = pSerialized->awcString + (uSizeWhole - uSizeOfValue) / sizeof(wchar_t);
		for(size_t uIndex = 0; uIndex < uSizeOfValue / sizeof(wchar_t); ++uIndex){
			MCFBUILD_move_be_uint16(pwcWriteBase + uIndex, pwcReadBase + uIndex);
		}
		pbyWrite += sizeof(SerializedElement) + uSizeWhole;
		// Scan the next element.
		uOffsetCursor += uCapacityTaken;
	}
	// Calculate the checksum.
	MCFBUILD_Sha256Context vSha256Context;
	MCFBUILD_Sha256Initialize(&vSha256Context);
	MCFBUILD_Sha256Update(&vSha256Context, kMagic, sizeof(kMagic));
	MCFBUILD_Sha256Update(&vSha256Context, pHeader->abyPayload, (size_t)(pbyWrite - pHeader->abyPayload));
	MCFBUILD_Sha256Finalize(&(pHeader->au8Checksum), &vSha256Context);
	// Hand over the buffer to our caller.
	*ppData = pHeader;
	*puSize = uSizeToAlloc;
	return true;
}
void MCFBUILD_VariableMapFreeSerializedBuffer(void *pData){
	MCFBUILD_HeapFree(pData);
}
bool MCFBUILD_VariableMapDeserialize(MCFBUILD_VariableMap *restrict pMap, const void *restrict pData, size_t uSize){
	if(uSize < sizeof(SerializedHeader)){
		MCFBUILD_SetLastError(ERROR_INVALID_DATA);
		return false;
	}
	const SerializedHeader *pHeader = pData;
	const unsigned char *pbyEnd = (const unsigned char *)pData + uSize;
	// Verify the checksum.
	uint8_t au8Checksum[32];
	MCFBUILD_Sha256Context vSha256Context;
	MCFBUILD_Sha256Initialize(&vSha256Context);
	MCFBUILD_Sha256Update(&vSha256Context, kMagic, sizeof(kMagic));
	MCFBUILD_Sha256Update(&vSha256Context, pHeader->abyPayload, (size_t)(pbyEnd - pHeader->abyPayload));
	MCFBUILD_Sha256Finalize(&au8Checksum, &vSha256Context);
	if(memcmp(au8Checksum, pHeader->au8Checksum, 32) != 0){
		MCFBUILD_SetLastError(ERROR_INVALID_DATA);
		return false;
	}
	// Collect serialized strings forwards, performing boundary checks on the way.
	size_t uMinimumSizeToReserve = 0;
	const unsigned char *pbyRead = pHeader->abyPayload;
	while(pbyRead != pbyEnd){
		if((size_t)(pbyEnd - pbyRead) < sizeof(SerializedElement)){
			MCFBUILD_SetLastError(ERROR_INVALID_DATA);
			return false;
		}
		const SerializedElement *pSerialized = (const void *)pbyRead;
		uint64_t u64SizeWholeSerialized = MCFBUILD_load_be_uint64(&(pSerialized->u64SizeWholeSerialized));
		uint64_t u64OffsetToValueSerialized = MCFBUILD_load_be_uint64(&(pSerialized->u64OffsetToValueSerialized));
		pbyRead += sizeof(SerializedElement);
		// See comments in `MCFBUILD_VariableMapSerialize()` for description of `SerializedElement::u64SizeWholeSerialized`.
		if(u64SizeWholeSerialized > PTRDIFF_MAX){
			MCFBUILD_SetLastError(ERROR_INVALID_DATA);
			return false;
		}
		size_t uSizeWholeSerialized = (size_t)u64SizeWholeSerialized;
		if((size_t)(pbyEnd - pbyRead) < uSizeWholeSerialized / 8 * 8){
			MCFBUILD_SetLastError(ERROR_INVALID_DATA);
			return false;
		}
		if(u64OffsetToValueSerialized > uSizeWholeSerialized / 8 * 8){
			MCFBUILD_SetLastError(ERROR_INVALID_DATA);
			return false;
		}
		size_t uOffsetToValueSerialized = (size_t)u64OffsetToValueSerialized;
		size_t uSizeOfKey = uOffsetToValueSerialized - uSizeWholeSerialized % 8;
		size_t uSizeOfValue = uSizeWholeSerialized / 8 * 8 - uOffsetToValueSerialized;
		// Add up the number of bytes that a MapElement is going to take.
		size_t uSizeWhole = uSizeOfKey + sizeof(wchar_t) + uSizeOfValue + sizeof(wchar_t);
		unsigned char bySizePadded = -uSizeWhole % 8;
		uSizeWhole += bySizePadded;
		if(__builtin_add_overflow(uMinimumSizeToReserve, sizeof(MapElement) + uSizeWhole, &uMinimumSizeToReserve)){
			MCFBUILD_SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			return false;
		}
		pbyRead += uSizeWholeSerialized / 8 * 8;
	}
	// Allocate the buffer. Reuse the existent buffer if possible.
	unsigned char *pbyStorage = pMap->pbyStorage;
	size_t uCapacity = pMap->uCapacity;
	if(uCapacity < uMinimumSizeToReserve){
		uCapacity = uMinimumSizeToReserve;
		pbyStorage = MCFBUILD_HeapRealloc(pbyStorage, uCapacity);
		if(!pbyStorage){
			return false;
		}
		pMap->pbyStorage = pbyStorage;
		pMap->uCapacity = uCapacity;
	}
	// Rebuild everything from scratch. Boundary checks are unnecessary this time.
	pMap->uOffsetEnd = 0;
	pbyRead = pHeader->abyPayload;
	while(pbyRead != pbyEnd){
		const SerializedElement *pSerialized = (const void *)pbyRead;
		size_t uSizeWholeSerialized = (size_t)MCFBUILD_load_be_uint64(&(pSerialized->u64SizeWholeSerialized));
		size_t uOffsetToValueSerialized = (size_t)MCFBUILD_load_be_uint64(&(pSerialized->u64OffsetToValueSerialized));
		pbyRead += sizeof(SerializedElement);
		// See comments in `MCFBUILD_VariableMapSerialize()` for description of `SerializedElement::u64SizeWholeSerialized`.
		size_t uSizeOfKey = uOffsetToValueSerialized - uSizeWholeSerialized % 8;
		size_t uSizeOfValue = uSizeWholeSerialized / 8 * 8 - uOffsetToValueSerialized;
		// Create a new element in the map.
		MapElement *pElement = (void *)(pbyStorage + pMap->uOffsetEnd);
		size_t uSizeWhole = uSizeOfKey + sizeof(wchar_t) + uSizeOfValue + sizeof(wchar_t);
		unsigned char bySizePadded = -uSizeWhole % 8;
		uSizeWhole += bySizePadded;
		size_t uOffsetToValue = uSizeWhole - uSizeOfValue - sizeof(wchar_t);
		pElement->uSizeWhole = uSizeWhole;
		pElement->uOffsetToValue = uOffsetToValue;
		// Load the key without the null terminator, then append one.
		const wchar_t *pwcReadBase = pSerialized->awcString;
		wchar_t *pwcWriteBase = pElement->awcString;
		for(size_t uIndex = 0; uIndex < uSizeOfKey / sizeof(wchar_t); ++uIndex){
			MCFBUILD_move_be_uint16(pwcWriteBase + uIndex, pwcReadBase + uIndex);
		}
		pwcWriteBase[uSizeOfKey / sizeof(wchar_t)] = 0;
		// Load the value without the null terminator, then append one.
		pwcReadBase = pSerialized->awcString + uOffsetToValueSerialized / sizeof(wchar_t);
		pwcWriteBase = pElement->awcString + uOffsetToValue / sizeof(wchar_t);
		for(size_t uIndex = 0; uIndex < uSizeOfValue / sizeof(wchar_t); ++uIndex){
			MCFBUILD_move_be_uint16(pwcWriteBase + uIndex, pwcReadBase + uIndex);
		}
		pwcWriteBase[uSizeOfValue / sizeof(wchar_t)] = 0;
		pbyRead += uSizeWholeSerialized / 8 * 8;
		pMap->uOffsetEnd += sizeof(MapElement) + uSizeWhole;
	}
	return true;
}
