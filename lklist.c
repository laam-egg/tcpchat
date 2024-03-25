/**
 * To run tests:
 * gcc -g -Wall -Wpointer-arith -DLK_RUN_TEST -o lklist lklist.c && ./lklist
 */

#include "lklist.h"
#include <string.h>

struct _LkNode {
    struct _LkNode* prev;
    struct _LkNode* next;
};

struct _LkList {
    LkNode* head;
    LkNode* tail;
    size_t size;
    size_t dataSize;
};

#ifndef LK_RELEASE
void _cz(LkList const* ll, size_t dataSize) {
    if (ll->dataSize != dataSize) {
        fprintf(stderr, "LkList: ERROR: Using a function with the wrong list. Review the data type.\n");
        exit(1);
    }
}
#endif

LkList* lkInit(size_t dataSize) {
    LkList* ll = (LkList*)malloc(sizeof(LkList));
    if (!ll) return NULL;
    ll->head = ll->tail = NULL;
    ll->size = 0;
    ll->dataSize = dataSize;
    return ll;
}

void lkDestroy(LkList* ll) {
    lkClear(ll);
    free((void*)ll);
}

LkNode* lkLocate(LkList const* ll, int pos) {
    // if (pos <= ll->size / 2) {
        LkNode* found = ll->head;
        for (int i = 0; i < pos; ++i) {
            if (found == NULL) return NULL;
            found = found->next;
        }
        return found;
    // } else {
    //     LkNode* found = ll->tail;
    //     for (int i = ll->size - 1; i > pos; --i) {
    //         if (found == NULL) return NULL;
    //         found = found->prev;
    //     }
    //     return found;
    // }
}

LkNode* lkHead(LkList const* ll) {
    return ll->head;
}

LkNode* lkTail(LkList const* ll) {
    return ll->tail;
}

LkNode* lkBefore(LkNode const* node) {
    return node->prev;
}

LkNode* lkAfter(LkNode const* node) {
    return node->next;
}

void* lkGetNodeDataPtr(LkList const* ll, LkNode* node) {
    char* ptr = (char*)node + sizeof(LkNode);
    return (void*)ptr;
}

void lkSetNodeData(LkList* ll, LkNode* node, void const* dataPtr) {
    memcpy(lkGetNodeDataPtr(ll, node), dataPtr, ll->dataSize);
}

bool lkInsert(LkList* ll, LkNode* where, void const* dataPtr) {
    LkNode* newNode = (LkNode*)malloc(sizeof(LkNode) + ll->dataSize);
    lkSetNodeData(ll, newNode, dataPtr);
    if (newNode == NULL) {
        return false;
    }

    if (ll->head == NULL) {
        newNode->next = newNode->prev = NULL;
    } else if (where == NULL) {
        newNode->prev = ll->tail;
        newNode->next = NULL;
        if (ll->tail) {
            ll->tail->next = newNode;
        }
    } else {
        newNode->next = where;
        newNode->prev = where->prev;
        if (where->prev != NULL) {
            where->prev->next = newNode;
        }
        where->prev = newNode;
    }

    if (newNode->next == NULL) {
        ll->tail = newNode;
    }
    if (newNode->prev == NULL) {
        ll->head = newNode;
    }
    ++ll->size;
    return true;
}

void lkRemove(LkList* ll, LkNode* which) {
    LkNode* before = which->prev;
    LkNode* after = which->next;

    if (before == NULL) {
        ll->head = after;
    } else {
        before->next = after;
    }

    if (after == NULL) {
        ll->tail = before;
    } else {
        after->prev = before;
    }

    free((void*)which);
    --ll->size;
}

bool lkNext(LkNode** currentNodePtr) {
    if (*currentNodePtr == NULL) return false;
    *currentNodePtr = (*currentNodePtr)->next;
    if (*currentNodePtr == NULL) return false;
    return true;
}

int lkFind(LkList* ll, bool (*predicate)(LkList*, LkNode*, void*), void* extraData) {
    if (ll->size == 0) return -1;

    LkNode* current = lkHead(ll);
    int i = 0;
    do {
        if (predicate(ll, current, extraData)) {
            return i;
        }
        ++i;
    } while (lkNext(&current));
    return -1;
}

int lkIndexOf(LkList* ll, void const* dataPtr) {
    if (ll->size == 0) return -1;

    LkNode* current = lkHead(ll);
    int i = 0;
    do {
        if (memcmp(lkGetNodeDataPtr(ll, current), dataPtr, ll->dataSize) == 0) {
            return i;
        }
        ++i;
    } while (lkNext(&current));
    return -1;
}

void lkClear(LkList* ll) {
    LkNode* current = lkHead(ll);
    LkNode* before = NULL;
    while (lkNext(&current)) {
        free((void*)before);
        before = current;
    }
    free((void*)current);

    ll->size = 0;
    ll->head = ll->tail = NULL;
}

size_t lkSize(LkList const* ll) {
    return ll->size;
}

void lkCopyToArray(LkList const* ll, void* dest) {
    if (lkSize(ll) == 0) return;

    size_t offset = 0;
    LkNode* current = lkHead(ll);
    do {
        memcpy((void*)((char*)dest + offset), lkGetNodeDataPtr(ll, current), ll->dataSize);
        offset += ll->dataSize;
    } while (lkNext(&current));
}

void* lkCopyToNewArray(LkList const* ll) {
    size_t arrSize = lkSize(ll) * ll->dataSize;
    if (arrSize == 0) return NULL;
    void* arr = malloc(lkSize(ll) * ll->dataSize);
    lkCopyToArray(ll, arr);
    return arr;
}

#ifdef LK_RUN_TEST
LK_WANT_PRIMITIVE_TYPE(int, Int_, )
LK_WANT_PRIMITIVE_TYPE(double, Double_, )

#include <stdio.h>
void printList(LkList* ll) {
    printf("Size = %zu\n", ll->size);
    LkNode* current = ll->head;
    while (current != NULL) {
        printf("%d ", lkInt_GetNodeData(ll, current));
        current = current->next;
    }
    printf("\n");
}

typedef struct {
    char name[256];
    int age;
} Student;

LK_WANT_STRUCT_TYPE(Student, Student_, )

int main() {
    LkInt_List* ll = lkInt_Init();
    lkInt_Insert(ll, lkInt_Locate(ll, 0), 1);
    lkInt_Insert(ll, lkInt_Locate(ll, 0), 2);
    lkInt_Insert(ll, lkInt_Locate(ll, 0), 3);
    lkInt_Insert(ll, NULL, 400);
    printList(ll); // Expected: 3 2 1 400

    lkInt_Remove(ll, lkInt_Locate(ll, 0));
    lkInt_Remove(ll, lkInt_Locate(ll, 1));
    printList(ll); // Expected: 2 400

    lkInt_Remove(ll, lkInt_Locate(ll, 1));
    printList(ll); // Expected: 2

    lkInt_SetNodeData(ll, lkInt_Locate(ll, 0), 500);
    printList(ll); // Expected: 500

    lkInt_Remove(ll, lkInt_Locate(ll, 0));
    printList(ll); // Expected: None

    lkInt_Insert(ll, NULL, 20);
    lkInt_Insert(ll, lkHead(ll), 40);
    printList(ll); // Expected: 40 20

    printf("Index of 20 = %d\n", lkInt_IndexOf(ll, 20)); // Expected: 1
    printf("Index of 40 = %d\n", lkInt_IndexOf(ll, 40)); // Expected: 0
    printf("Index of 60 = %d\n", lkInt_IndexOf(ll, 60)); // Expected: -1
    
    lkInt_Clear(ll);
    printList(ll); // Expected: None

    lkInt_Insert(ll, NULL, -4);
    lkInt_Insert(ll, NULL, -6);
    lkInt_Insert(ll, lkHead(ll), 0);
    printf("Convert to array: ");
    int* arr = lkInt_CopyToNewArray(ll);
    for (size_t i = 0; i < lkSize(ll); ++i) {
        printf("%d ", arr[i]);
    }
    printf("\n"); // Expected: 0 -4 -6
    free((void*)arr);

    // lkDouble_Clear(ll); // Expected: ERROR
    // lkDouble_CopyToNewArray(ll); // Expected: ERROR

    lkDestroy(ll);

    printf("TEST DONE.\n");
}
#endif // LK_RUN_TEST
