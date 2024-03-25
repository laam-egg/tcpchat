#ifndef LkList_INCLUDED
#define LkList_INCLUDED

#include <stdlib.h>
#include <stdbool.h>

typedef struct _LkNode LkNode;
typedef struct _LkList LkList;

LkList* lkInit(size_t dataSize);
void lkDestroy(LkList* ll);
void lkClear(LkList* ll);
LkNode* lkLocate(LkList const* ll, int pos);
LkNode* lkHead(LkList const* ll);
LkNode* lkTail(LkList const* ll);
bool lkNext(LkNode** currentNodePtr);
LkNode* lkBefore(LkNode const* node);
LkNode* lkAfter(LkNode const* node);
void* lkGetNodeDataPtr(LkList const* ll, LkNode* node);
void lkSetNodeData(LkList* ll, LkNode* node, void const* dataPtr);
bool lkInsert(LkList* ll, LkNode* where, void const* dataPtr);
void lkRemove(LkList* ll, LkNode* which);
int lkFind(LkList* ll, bool (*predicate)(LkList*, LkNode*, void*), void* extraData);
int lkIndexOf(LkList* ll, void const* dataPtr);
size_t lkSize(LkList const* ll);
void lkCopyToArray(LkList const* ll, void* dest);
void* lkCopyToNewArray(LkList const* ll);

#define TEMPLATE_FUNCTION

#ifndef LK_RELEASE
#include <stdio.h>
// CZ = Check Data Size
#define CZ(ll, T) _cz(ll, sizeof(T));
void _cz(LkList const* ll, size_t dataSize);
#else
#define CZ(ll, T)
#endif

#define LK_WANT_PRIMITIVE_TYPE(T, PREFIX, SUFFIX) \
typedef LkList Lk##PREFIX##List##SUFFIX; \
typedef LkNode Lk##PREFIX##Node##SUFFIX; \
TEMPLATE_FUNCTION LkList* lk##PREFIX##Init##SUFFIX() { \
    return lkInit(sizeof(T)); \
} \
TEMPLATE_FUNCTION void lk##PREFIX##Clear##SUFFIX(LkList* ll) { \
    CZ(ll, T) \
    return lkClear(ll); \
} \
TEMPLATE_FUNCTION void lk##PREFIX##Destroy##SUFFIX(LkList* ll) { \
    CZ(ll, T) \
    return lkDestroy(ll); \
} \
TEMPLATE_FUNCTION LkNode* lk##PREFIX##Locate##SUFFIX(LkList const* ll, int pos) { \
    CZ(ll, T) \
    return lkLocate(ll, pos); \
} \
TEMPLATE_FUNCTION LkNode* lk##PREFIX##Head##SUFFIX(LkList const* ll) { \
    CZ(ll, T) \
    return lkHead(ll); \
} \
TEMPLATE_FUNCTION LkNode* lk##PREFIX##Tail##SUFFIX(LkList const* ll) { \
    CZ(ll, T) \
    return lkTail(ll); \
} \
TEMPLATE_FUNCTION LkNode* lk##PREFIX##Before##SUFFIX(LkNode const* node) { \
    return lkBefore(node); \
} \
TEMPLATE_FUNCTION LkNode* lk##PREFIX##After##SUFFIX(LkNode const* node) { \
    return lkAfter(node); \
} \
TEMPLATE_FUNCTION bool lk##PREFIX##Next##SUFFIX(LkNode** currentNodePtr) { \
    return lkNext(currentNodePtr); \
} \
TEMPLATE_FUNCTION T lk##PREFIX##GetNodeData##SUFFIX(LkList const* ll, LkNode* node) { \
    CZ(ll, T) \
    return *(T*)lkGetNodeDataPtr(ll, node); \
} \
TEMPLATE_FUNCTION void lk##PREFIX##SetNodeData##SUFFIX(LkList* ll, LkNode* node, T const data) { \
    CZ(ll, T) \
    lkSetNodeData(ll, node, (void*)&data); \
} \
TEMPLATE_FUNCTION bool lk##PREFIX##Insert##SUFFIX(LkList* ll, LkNode* where, T const k) { \
    CZ(ll, T) \
    return lkInsert(ll, where, (void const*)&k); \
} \
TEMPLATE_FUNCTION void lk##PREFIX##Remove##SUFFIX(LkList* ll, LkNode* which) { \
    CZ(ll, T) \
    return lkRemove(ll, which); \
} \
TEMPLATE_FUNCTION int lk##PREFIX##Find##SUFFIX(LkList* ll, bool (*predicate)(LkList*, LkNode*, void*), void* extraData) { \
    CZ(ll, T) \
    return lkFind(ll, predicate, extraData); \
} \
TEMPLATE_FUNCTION int lk##PREFIX##IndexOf##SUFFIX(LkList* ll, T const data) { \
    CZ(ll, T) \
    return lkIndexOf(ll, (void const*)&data); \
} \
TEMPLATE_FUNCTION size_t lk##PREFIX##Size##SUFFIX(LkList const* ll) { \
    CZ(ll, T) \
    return lkSize(ll); \
} \
TEMPLATE_FUNCTION void lk##PREFIX##CopyToArray##SUFFIX(LkList const* ll, T* dest) { \
    CZ(ll, T) \
    return lkCopyToArray(ll, (void*)dest); \
} \
TEMPLATE_FUNCTION T* lk##PREFIX##CopyToNewArray##SUFFIX(LkList const* ll) { \
    CZ(ll, T) \
    return (T*)lkCopyToNewArray(ll); \
} \



#define LK_WANT_STRUCT_TYPE(T, PREFIX, SUFFIX) \
typedef LkList Lk##PREFIX##List##SUFFIX; \
typedef LkNode Lk##PREFIX##Node##SUFFIX; \
TEMPLATE_FUNCTION LkList* lk##PREFIX##Init##SUFFIX() { \
    return lkInit(sizeof(T)); \
} \
TEMPLATE_FUNCTION void lk##PREFIX##Clear##SUFFIX(LkList* ll) { \
    CZ(ll, T) \
    return lkClear(ll); \
} \
TEMPLATE_FUNCTION void lk##PREFIX##Destroy##SUFFIX(LkList* ll) { \
    CZ(ll, T) \
    return lkDestroy(ll); \
} \
TEMPLATE_FUNCTION LkNode* lk##PREFIX##Locate##SUFsFIX(LkList const* ll, int pos) { \
    CZ(ll, T) \
    return lkLocate(ll, pos); \
} \
TEMPLATE_FUNCTION LkNode* lk##PREFIX##Head##SUFFIX(LkList const* ll) { \
    CZ(ll, T) \
    return lkHead(ll); \
} \
TEMPLATE_FUNCTION LkNode* lk##PREFIX##Tail##SUFFIX(LkList const* ll) { \
    CZ(ll, T) \
    return lkTail(ll); \
} \
TEMPLATE_FUNCTION LkNode* lk##PREFIX##Before##SUFFIX(LkNode const* node) { \
    return lkBefore(node); \
} \
TEMPLATE_FUNCTION LkNode* lk##PREFIX##After##SUFFIX(LkNode const* node) { \
    return lkAfter(node); \
} \
TEMPLATE_FUNCTION bool lk##PREFIX##Next##SUFFIX(LkNode** currentNodePtr) { \
    return lkNext(currentNodePtr); \
} \
TEMPLATE_FUNCTION T* lk##PREFIX##GetNodeData##SUFFIX(LkList const* ll, LkNode* node) { \
    CZ(ll, T) \
    return (T*)lkGetNodeDataPtr(ll, node); \
} \
TEMPLATE_FUNCTION void lk##PREFIX##SetNodeData##SUFFIX(LkList* ll, LkNode* node, T const* const dataPtr) { \
    CZ(ll, T) \
    lkSetNodeData(ll, node, (void*)dataPtr); \
} \
TEMPLATE_FUNCTION bool lk##PREFIX##Insert##SUFFIX(LkList* ll, LkNode* where, T const* const dataPtr) { \
    CZ(ll, T) \
    return lkInsert(ll, where, (void const*)dataPtr); \
} \
TEMPLATE_FUNCTION void lk##PREFIX##Remove##SUFFIX(LkList* ll, LkNode* which) { \
    CZ(ll, T) \
    return lkRemove(ll, which); \
} \
TEMPLATE_FUNCTION int lk##PREFIX##Find##SUFFIX(LkList* ll, bool (*predicate)(LkList*, LkNode*, void*), void* extraData) { \
    CZ(ll, T) \
    return lkFind(ll, predicate, extraData); \
} \
TEMPLATE_FUNCTION int lk##PREFIX##IndexOf##SUFFIX(LkList* ll, T const* const dataPtr) { \
    CZ(ll, T) \
    return lkIndexOf(ll, (void const*)dataPtr); \
} \
TEMPLATE_FUNCTION size_t lk##PREFIX##Size##SUFFIX(LkList const* ll) { \
    CZ(ll, T) \
    return lkSize(ll); \
} \
TEMPLATE_FUNCTION void lk##PREFIX##CopyToArray##SUFFIX(LkList const* ll, T* dest) { \
    CZ(ll, T) \
    return lkCopyToArray(ll, (void*)dest); \
} \
TEMPLATE_FUNCTION T* lk##PREFIX##CopyToNewArray##SUFFIX(LkList const* ll) { \
    CZ(ll, T) \
    return (T*)lkCopyToNewArray(ll); \
} \

#endif // LkList_INCLUDED
