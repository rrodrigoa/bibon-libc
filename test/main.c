
#if defined(_WIN32)
    #include "/Source/LocalGit/TLSFLib/src/tlsf.h"
#else
    #include "tlsf.h"
#endif

struct Test2Struct{
    float floatValue;
    int intValue;
    struct Test2Struct *next;
    char charStr[9];
};

void basicTest(){
    printf("Test 1\nWill allocate memory for 9 chars\n");

    unsigned int bytes = 9;
    char* myCharArray = (char*)__libc_malloc(bytes);

    if (myCharArray == NULL) {
        printf("Pointer failed to be allocated, returned NULL\n");
        return;
    }

    myCharArray[0] = 'D';
    myCharArray[1] = 'E';
    myCharArray[2] = 'A';
    myCharArray[3] = 'D';
    myCharArray[4] = 'B';
    myCharArray[5] = 'E';
    myCharArray[6] = 'E';
    myCharArray[7] = 'F';
    myCharArray[8] = '\0';

    printf("Allocated char value is [%s]\n", myCharArray);

    printf("Free allocated memory\n");
    __libc_free(myCharArray);

    printf("\nTest 2\nWill allocate memory for 2 structure objs\n");
    struct Test2Struct* head = (struct Test2Struct*)__libc_malloc(sizeof(struct Test2Struct));
    struct Test2Struct* next = (struct Test2Struct*)__libc_malloc(sizeof(struct Test2Struct));

    head->charStr[0] = 'D';
    head->charStr[1] = 'E';
    head->charStr[2] = 'A';
    head->charStr[3] = 'D';
    head->charStr[4] = 'B';
    head->charStr[5] = 'E';
    head->charStr[6] = 'E';
    head->charStr[7] = 'F';
    head->charStr[8] = '\0';

    head->intValue = 8;
    head->floatValue = 5.3f;

    next->charStr[0] = 'B';
    next->charStr[1] = 'E';
    next->charStr[2] = 'E';
    next->charStr[3] = 'F';
    next->charStr[4] = 'D';
    next->charStr[5] = 'E';
    next->charStr[6] = 'A';
    next->charStr[7] = 'D';
    next->charStr[8] = '\0';

    next->intValue = 16;
    next->floatValue = 2.0f;

    head->next = next;
    next->next = NULL;
    struct Test2Struct *pt = head;

    while(pt != NULL){
        printf("charStr value is [%s]\n", pt->charStr);
        printf("intValue value is [%d]\n", pt->intValue);
        printf("floatVlaue value is [%f]\n", pt->floatValue);
        pt = pt->next;
        if(pt != NULL){
            printf("going to next struct\n");
        }
    }

    printf("free two structs\n");

    __libc_free(head);
    __libc_free(next);

    printf("DONE\n");
}

ALIGNAS(64) struct TestStructAligned {
    char data[32];
};

void memalignTest() {
    printf("Memalign Test 1: Allocate aligned memory for 32 bytes (alignment = 64)\n");
    size_t alignment = 64;
    size_t size = 32;

    void* aligned_ptr = __libc_memalign(alignment, size);

    if (aligned_ptr == NULL) {
        printf("memalign returned nullptr!\n");
        return;
    }

    // Verifica alinhamento
    if (((uintptr_t)aligned_ptr % alignment) != 0) {
        printf("Alignment failed! Address is not aligned to %d bytes\n", alignment);
        __libc_free(aligned_ptr);
        assert(false && "Aligned pointer is not properly aligned");
    }

    printf("Pointer is aligned to %zu bytes: %p\n", alignment, aligned_ptr);

    // Testa escrita e leitura
    snprintf((char*)aligned_ptr, 32, "%s", "DEAD BEEF OK!");
    printf("Stored string: [%s]\n", (char*)aligned_ptr);

    __libc_free(aligned_ptr);
    printf("Memory freed successfully.\n");

    // Repetir com outro alinhamento (ex: 128)
    printf("\nMemalign Test 2: alignment = 128\n");

    alignment = 128;
    aligned_ptr = __libc_memalign(size, alignment);

    assert(aligned_ptr != NULL);
    assert(((uintptr_t)aligned_ptr % alignment) == 0);

    memset(aligned_ptr, 0xAB, size);
    printf("Pointer aligned to 128 bytes and memory set successfully.\n");

    __libc_free(aligned_ptr);
    printf("Second memory block freed.\n");

    printf("Memalign tests finished successfully.\n");
}

int main() {
    basicTest();
    memalignTest();

    return 0;
}

