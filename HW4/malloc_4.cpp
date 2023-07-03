#include <unistd.h>
#include <cstring>
#include <stdlib.h>
#include <sys/mman.h>
#include <cstdlib>
#include <ctime>
#include <stdio.h>

#define MAX_ORDER 10
#define SIZE_128K 131072

size_t _num_allocated_blocks();

typedef struct MallocMetadata {
    int cookie; 
    size_t size; // size includes the size of the metadata
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
    void* address; //address is the address of the data
    // MallocMetadata* buddy;
    bool is_mmap;
    // bool is_huge;
} MallocMetadata;

int globalCookie;

void initMallocMetaData(MallocMetadata* metadata, size_t size1, bool is_free1,
             MallocMetadata* next1, MallocMetadata* prev1, void* address1)
{
    metadata->cookie = globalCookie;
    metadata->size = size1;
    metadata->is_free = is_free1;
    metadata->next = next1;
    metadata->prev = prev1;
    metadata->address = address1;
    metadata->is_mmap = false;
}

void checkBufferOverflow(MallocMetadata* metadata)
{
    if(metadata->cookie != globalCookie)
    {
        exit(0xdeadbeef);
    }
}

class BlockList 
{    
public:
    MallocMetadata* head;
    MallocMetadata* tail;
    size_t list_size;

    BlockList(): head(nullptr), tail(nullptr), list_size(0) {}

    void insertOrdered(MallocMetadata* newNode)
    {
        if(head == nullptr) {
            head = newNode;
            tail = newNode;
            return;
        }
        MallocMetadata* temp = head;
        MallocMetadata* prevTemp = nullptr;
        while(temp != nullptr) 
        {
            checkBufferOverflow(temp);   
            if(newNode->address < temp->address) 
            {
                break;
            }
            prevTemp = temp;
            temp = temp->next;
        }
        // if the new node is the biggest
        if(temp == nullptr)
        {
            prevTemp->next = newNode;
            newNode->prev = prevTemp;
            tail = newNode;
        }
        else if(temp == head) // if the new node is the biggest
        {
            newNode->next = head;
            head->prev = newNode;
            head = newNode;
        }
        else // if the new node is in the middle
        {
            prevTemp->next = newNode;
            newNode->prev = prevTemp;
            newNode->next = temp;
            temp->prev = newNode;
        }
        
    }
    
    void deleteOrdered(MallocMetadata* newData){
        MallocMetadata* current = head;
        MallocMetadata* prev = nullptr;

        while(current != nullptr && current->address != newData->address){
            checkBufferOverflow(current);
            prev = current;
            current = current->next;
        }

        // new data wasn't found in the list
        if(!current){
            return;
        }

        // in case new data was found and it is the first element in the list
        if(current == head){
            if(head == tail){ // only one element in the list
                tail = nullptr;
                head = nullptr;
            }
            else{
                head = head->next;
                head->prev = nullptr;
            }
            
        }
        // in case new data was found and it is not the first element in the list
        else{
            if(tail == current){
                tail=prev;
                prev->next= nullptr;
            }
            else{
                prev->next = current->next;
                (current->next)->prev = prev;
            }
        }

        current->next = nullptr;
        current->prev = nullptr;
        
    }

    void* findFreeBySize(size_t size){
        MallocMetadata* current = head;
        while(current)
        {
            checkBufferOverflow(current);
            if(current->size >= size && current->is_free)
            {
                current->is_free = false;
                return current->address;
            }
            current = current->next;
        }
        return nullptr;
    }

    void* findByAddress(void* address){
        MallocMetadata* current = head;
        while(current != nullptr){
            checkBufferOverflow(current);
            if(current->address == address)
            {
                return current->address;
            }
            current = current->next;
        }
        return nullptr;
    }

    size_t getFreeCount()
    {
        size_t cnt = 0;
        MallocMetadata* current = head;
        while(current != nullptr)
        {
            checkBufferOverflow(current);
            if(current->is_free)
            {
                cnt++;
            }
            current = current->next;
        }
        return cnt;
    }

    size_t getFreeBytesCount()
    {
        size_t cnt = 0;
        MallocMetadata* current = head;
        while(current != nullptr)
        {
            checkBufferOverflow(current);
            if(current->is_free)
            {
                cnt += current->size - sizeof(MallocMetadata);
            }
            current = current->next;
        }
        return cnt;
    }

    size_t getAllocatedBlocks()
    {
        size_t cnt = 0;
        MallocMetadata* current = head;
        while(current != nullptr)
        {
            checkBufferOverflow(current);
            cnt++;
            current = current->next;
        }
        return cnt;
    }

    size_t getAllocatedBytesCount()
    {
        size_t cnt = 0;
        MallocMetadata* current = head;
        while(current != nullptr)
        {
            checkBufferOverflow(current);
            cnt += current->size - sizeof(MallocMetadata);
            current = current->next;
        }
        return cnt;
    }

    bool isInList(MallocMetadata* current)
    {
        MallocMetadata* temp = head;
        while(temp != nullptr)
        {
            checkBufferOverflow(temp);
            if(temp == current)
            {
                return true;
            }
            temp = temp->next;
        }
        return false;
    }
};


/*
0 - 128 
1 - 256
2 - 512 
3 - 1024 (1K)
4 - 2048 (2K)
5 - 4096 (4K)
6 - 8192 (8K)
7 - 16384 (16K)
8 - 32768 (32K)
9 - 65536 (64K)
10 - 131072 (128K)
*/

// challenge 0
BlockList listsArr[MAX_ORDER+1];
bool firstMalloc = true;
size_t sizeOfBlocks[MAX_ORDER+1] = {128, 256, 512, 1024, 2048, 4096, 8192, 16384,
                               32768, 65536, 131072};
BlockList mmapList;

int findListIndex(size_t size)
{
    for(int i = 0; i <= MAX_ORDER; i++)
    {
        if(sizeOfBlocks[i] == size)
        {
            return i;
        }
    }
    return -1;
}

void blockPartition(MallocMetadata* metaRes, int index, int partitionNumber)
{
    if(partitionNumber == 0)
    {
        return;
    }
    listsArr[index].deleteOrdered(metaRes);
    while(partitionNumber > 0)
    {
        MallocMetadata* newBlock = reinterpret_cast<MallocMetadata*>(reinterpret_cast<char*>(metaRes) + sizeOfBlocks[index-1]);
        initMallocMetaData(newBlock, sizeOfBlocks[index-1], true, nullptr, nullptr,
                reinterpret_cast<void*>(reinterpret_cast<char*>(metaRes) + sizeOfBlocks[index-1] + sizeof(MallocMetadata)));
        listsArr[index-1].insertOrdered(newBlock);
        partitionNumber--;
        index--;
    }
    MallocMetadata* firstNewBlock = metaRes;
    initMallocMetaData(firstNewBlock, sizeOfBlocks[index], false, nullptr, nullptr, metaRes->address);
    listsArr[index].insertOrdered(firstNewBlock);
}

void* findTightestBlock(size_t requestedSize){
    void* res;
    int cntFit = 0;
    for(int i=0; i<MAX_ORDER+1; i++)
    {
        if(sizeOfBlocks[i] >= requestedSize)
        {
            res = listsArr[i].findFreeBySize(requestedSize);
            
            if(res != nullptr)
            {
                MallocMetadata *metaRes = reinterpret_cast<MallocMetadata*>(reinterpret_cast<char*>(res) - sizeof(MallocMetadata));
                blockPartition(metaRes, i, cntFit);
                metaRes->is_free = false;
                return reinterpret_cast<MallocMetadata*>(reinterpret_cast<char*>(metaRes) + sizeof(MallocMetadata));
            }
            cntFit++;
        }
    }
    return nullptr;
}


void* findBuddyAddress(MallocMetadata* metaData){
    checkBufferOverflow(metaData);
    unsigned long long ptrValue = reinterpret_cast<unsigned long long>(metaData);
    return reinterpret_cast<void*>(ptrValue ^ metaData->size);
}


void* smalloc(size_t size)
{
    if(firstMalloc)
    {
        srand(time(NULL));
        globalCookie = rand();

        firstMalloc = false;
        void* prevRes = sbrk(0);
        //align to 128K
        void * res = sbrk(32 * SIZE_128K - ((unsigned long long)prevRes%SIZE_128K) + SIZE_128K);
        //res = (char*) res + SIZE_128K - ((unsigned long long)prevRes%SIZE_128K);
        res = reinterpret_cast<MallocMetadata*>(reinterpret_cast<char*>(res) +  SIZE_128K - ((unsigned long long)prevRes%SIZE_128K));
   
        for(int i=0; i<MAX_ORDER+1; i++)
        {
            listsArr[i] = BlockList();
        }
        for(int i = 0; i <= 31; i++)
        {
            MallocMetadata* newMetaData = (MallocMetadata*) res;
            initMallocMetaData(newMetaData, SIZE_128K, true, nullptr, nullptr,
                     reinterpret_cast<MallocMetadata*>(reinterpret_cast<char*>(res) + sizeof(MallocMetadata)));
            listsArr[MAX_ORDER].insertOrdered(newMetaData);
            res = (char *) res + SIZE_128K;
        }

        mmapList = BlockList();
    }

    if(size == 0 || size > 100000000)
    {
        return NULL;
    }



    if(size +sizeof(MallocMetadata) <= SIZE_128K)
    {
        void* tightestBlock = findTightestBlock(size + sizeof(MallocMetadata));
        if(tightestBlock != nullptr)
        {
            return tightestBlock;
        }
        return NULL;
    }

    //mmap case
// 1MB - 1,048,576 
// 4MB - 4,194,304

    void* res;
    //is hugepage
    if(size > 4194304)
    {
        res = mmap(NULL, size+sizeof(MallocMetadata), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    }
    else
    {
        res = mmap(NULL, size + sizeof(MallocMetadata), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    }

    MallocMetadata* mmapMetaData = (MallocMetadata*)res;
    res = reinterpret_cast<MallocMetadata*>(reinterpret_cast<char*>(res) + sizeof(MallocMetadata));
    initMallocMetaData(mmapMetaData, size + sizeof(MallocMetadata), false
                        , nullptr, nullptr, res);  
    mmapMetaData->is_mmap = true; 
    mmapMetaData->cookie = globalCookie;
    mmapList.insertOrdered(mmapMetaData);
    return mmapMetaData->address;
}


void* smalloc_for_scalloc(size_t size)
{
    if(firstMalloc)
    {
        srand(time(NULL));
        globalCookie = rand();

        firstMalloc = false;
        void* prevRes = sbrk(0);
        //align to 128K
        void * res = sbrk(32 * SIZE_128K - ((unsigned long long)prevRes%SIZE_128K) + SIZE_128K);
        //res = (char*) res + SIZE_128K - ((unsigned long long)prevRes%SIZE_128K);
        res = reinterpret_cast<MallocMetadata*>(reinterpret_cast<char*>(res) +  SIZE_128K - ((unsigned long long)prevRes%SIZE_128K));
   
        for(int i=0; i<MAX_ORDER+1; i++)
        {
            listsArr[i] = BlockList();
        }
        for(int i = 0; i <= 31; i++)
        {
            MallocMetadata* newMetaData = (MallocMetadata*) res;
            initMallocMetaData(newMetaData, SIZE_128K, true, nullptr, nullptr,
                     reinterpret_cast<MallocMetadata*>(reinterpret_cast<char*>(res) + sizeof(MallocMetadata)));
            listsArr[MAX_ORDER].insertOrdered(newMetaData);
            res = (char *) res + SIZE_128K;
        }

        mmapList = BlockList();
    }

    if(size == 0 || size > 100000000)
    {
        return NULL;
    }



    if(size +sizeof(MallocMetadata) <= SIZE_128K)
    {
        void* tightestBlock = findTightestBlock(size + sizeof(MallocMetadata));
        if(tightestBlock != nullptr)
        {
            return tightestBlock;
        }
        return NULL;
    }

    //mmap case
// 1MB - 1,048,576 
// 2MB - 2,097,152
// 4MB - 4,194,304

    void* res;
    //is hugepage
    if(size > 2097152)
    {
        res = mmap(NULL, size+sizeof(MallocMetadata), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    }
    else
    {
        res = mmap(NULL, size + sizeof(MallocMetadata), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    }

    MallocMetadata* mmapMetaData = (MallocMetadata*)res;
    res = reinterpret_cast<MallocMetadata*>(reinterpret_cast<char*>(res) + sizeof(MallocMetadata));
    initMallocMetaData(mmapMetaData, size + sizeof(MallocMetadata), false
                        , nullptr, nullptr, res);  
    mmapMetaData->is_mmap = true; 
    mmapMetaData->cookie = globalCookie;
    mmapList.insertOrdered(mmapMetaData);
    return mmapMetaData->address;
}

void* scalloc(size_t num, size_t size){
    if(size == 0 || num == 0 || size*num > 100000000){
        return NULL;
    }
    void* blockPointer;
    //if size is bigger than 2MB
    if(size > 2097152)
    {
        blockPointer = smalloc_for_scalloc(num*size);
    }
    else
    {
        blockPointer = smalloc(num*size);
    }

    if(blockPointer == NULL){
        return NULL;
    }     
    std::memset(blockPointer, 0, num*size);      
    return blockPointer;
}

void sfree(void* p){
    if(p == NULL){
        return;
    }
    MallocMetadata *current = reinterpret_cast<MallocMetadata*>(reinterpret_cast<char*>(p) - sizeof(MallocMetadata));
    checkBufferOverflow(current);
    if(current->is_free){
        return;
    }
    if(current->size > SIZE_128K || current->is_mmap)
    {
        mmapList.deleteOrdered(current);
        munmap(current, current->size);
        return;
    }

    current->is_free = true;
    if(current->size == SIZE_128K){
        return;
    }
    MallocMetadata* buddy = (MallocMetadata*) findBuddyAddress(current);
    checkBufferOverflow(buddy);
    current->is_free = true;
    if(!buddy->is_free)
    {
        return;
    }

    while(buddy->is_free && current->size < sizeOfBlocks[MAX_ORDER])
    {
        checkBufferOverflow(buddy);
        int i = findListIndex(current->size);
        listsArr[i].deleteOrdered(current);
        listsArr[i].deleteOrdered(buddy);

        if(buddy->address < current->address)
        {
            current = buddy;
        }

        current->size = current->size * 2;
        current->is_free = true;
        listsArr[i+1].insertOrdered(current);  
        
        if(current->size == SIZE_128K)
        {
            break;
        }
        buddy = (MallocMetadata*) findBuddyAddress(current);
        
    }
}


void* srealloc(void* oldp, size_t size){
    if(size == 0){
        return NULL;
    }
    
    if(oldp == nullptr){
        return smalloc(size);
    }

    MallocMetadata *current = reinterpret_cast<MallocMetadata*>(reinterpret_cast<char*>(oldp) - sizeof(MallocMetadata));
    checkBufferOverflow(current);
    if(current->size == size+sizeof(MallocMetadata) || 
                (current->size >= size+sizeof(MallocMetadata) && !current->is_mmap)){
        return oldp;
    }

    void* newBlock;
    MallocMetadata* buddy = (MallocMetadata*) findBuddyAddress(current);
    if(buddy->is_free)
    {
        checkBufferOverflow(buddy);
        while(buddy->is_free && buddy->size < sizeOfBlocks[MAX_ORDER])
        {
            int i = findListIndex(current->size);
            listsArr[i].deleteOrdered(current);
            listsArr[i].deleteOrdered(buddy);
            current->size *= 2;
            listsArr[i+1].insertOrdered(current);
            buddy = (MallocMetadata*) findBuddyAddress(current);

            if(current->size>= size + sizeof(MallocMetadata)){
                newBlock = current->address;
                break;
            }
        }
    }
    else
    {
        current->is_free = true;
        newBlock = smalloc(size);
        if(newBlock == NULL){
            current->is_free = false;
            return NULL;
        }
    }
    std::memmove(newBlock, oldp, current->size);
    if(current->is_mmap)
    {
        munmap(current, current->size);
        mmapList.deleteOrdered(current);
    }
    return newBlock;   
}


size_t _num_free_blocks(){
    int blockCnt = 0;
    for(int i = 0; i < MAX_ORDER+1; i++)
    {
        blockCnt += listsArr[i].getFreeCount();
    }
    return blockCnt;
}

size_t _num_free_bytes(){
    size_t totalSize = 0;
    for(int i = 0; i < MAX_ORDER+1; i++)
    {
        totalSize += listsArr[i].getFreeBytesCount();
    }
    return totalSize;
}

size_t _num_allocated_blocks(){
    int blockCnt = 0;
    for(int i = 0; i < MAX_ORDER+1; i++)
    {
        blockCnt += listsArr[i].getAllocatedBlocks();
    }

    blockCnt += mmapList.getAllocatedBlocks();
    return blockCnt;
}


size_t _num_allocated_bytes(){
    int bytesCnt = 0;
    for(int i = 0; i < MAX_ORDER+1; i++)
    {
        bytesCnt += listsArr[i].getAllocatedBytesCount();
    }

    bytesCnt += mmapList.getAllocatedBytesCount();
    return bytesCnt;
}

 size_t _num_meta_data_bytes(){
    return _num_allocated_blocks() * sizeof(MallocMetadata);
    
 }

size_t _size_meta_data(){
    return sizeof(MallocMetadata);
 }
