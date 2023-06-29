#include <unistd.h>
#include <cstring>


typedef struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
    void* address;
} MallocMetadata;

void initMallocMetaData(MallocMetadata* metadata, size_t size1, bool is_free1,
             MallocMetadata* next1, MallocMetadata* prev1, void* address1)
{
    metadata->size = size1;
    metadata->is_free = is_free1;
    metadata->next = next1;
    metadata->prev = prev1;
    metadata->address = address1;
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
            //
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

        while(current && current->address != newData->address){
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
                tail=nullptr;
            }
            prev->next = current->next;
            (current->next)->prev = prev;
        }
        
    }

    void* findFreeBySize(size_t size){
        MallocMetadata* current = head;
        while(current){
            if(current->size >= size && current->is_free)
            {
                current->is_free = false;
                return current->address;
            }
            current = current->next;
        }
        return nullptr;
    }
};

BlockList mallocDataList = BlockList(); 

void* smalloc(size_t size)
{
    if(size == 0 || size > 100000000)
    {
        return NULL;
    }
    void* res= mallocDataList.findFreeBySize(size);
    if(res != nullptr){
        return res;
    }
    res = sbrk(size+ sizeof(MallocMetadata));
    if(res == (void*)(-1))
    {
        return NULL;
    }
    MallocMetadata* newMetaData = (MallocMetadata*) res;
    res = (void *) res + sizeof(MallocMetadata);

    initMallocMetaData(newMetaData, size, false, nullptr, nullptr, res);   
    mallocDataList.insertOrdered(newMetaData);   
    return newMetaData;
}

void* scalloc(size_t num, size_t size){
    if(size == 0 || num == 0 || size*num > 100000000){
        return NULL;
    }
    
    void* blockPointer = smalloc(num*size);
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
    MallocMetadata *toFree = nullptr;
    MallocMetadata *current = mallocDataList.head;
    while(current && current->address != p){
        current = current->next;
    }
    if(!current){
        return;
    }
    current->is_free = true;         
}


void* srealloc(void* oldp, size_t size){
    if(size == 0){
        return NULL;
    }
    
    if(oldp == nullptr){
        return smalloc(size);
    }

    MallocMetadata* current = (MallocMetadata*) oldp - sizeof(MallocMetadata);
    if(current->size >= size){
        return oldp;
    }
    current->is_free = true;
    void* newBlock = smalloc(size);
    if(newBlock == NULL){
        current->is_free = false;
        return NULL;
    }
    std::memmove(newBlock, oldp, current->size);  
    return newBlock;   
}

size_t _num_free_blocks(){
    size_t count = 0;
    struct MallocMetadata* current = mallocDataList.head;
    while(current){
        if(current->is_free){
            count++;
        }
        current = current->next;
    } 
    return count;
}

size_t _num_free_bytes(){
    size_t totalSize = 0;
    struct MallocMetadata* current = mallocDataList.head;
    while(current){
        if(current->is_free){
            totalSize += current->size;            
        }
        current = current->next;
    } 
    return totalSize;
}

size_t _num_allocated_blocks(){
    size_t count = 0;
    struct MallocMetadata* current = mallocDataList.head;
    while(current){
        count++;
        current = current->next;
    }
    return count;
}


size_t _num_allocated_bytes(){
    size_t count = 0;
    struct MallocMetadata* current = mallocDataList.head;
    while(current){
        count += current->size;
        current = current->next;
    }
    return count;
}

size_t _num_meta_data_bytes(){
    return _num_allocated_blocks() * sizeof(MallocMetadata);
    
 }

size_t _size_meta_data(){
    return sizeof(MallocMetadata);
 }