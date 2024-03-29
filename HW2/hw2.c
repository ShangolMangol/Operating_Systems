#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/module.h>

asmlinkage long sys_hello(void) {
    printk("Hello, World!\n");
    return 0;
}

asmlinkage long sys_set_weight(int weight)
{
    if(weight < 0)
    {
        return -EINVAL;
    }
    current->weight = weight;
    return 0;
}

asmlinkage long sys_get_weight(void)
{
    return current->weight;
}

asmlinkage long sys_get_ancestor_sum(void)
{
    int sum = 0;
    struct task_struct *curr = current;
    while(curr->pid != 1){
        sum += curr->weight;
        curr = curr->parent;
    }

    // adding init weight
    sum += curr->weight;
    return sum;
}


struct task_struct* get_heaviest_descendant(struct task_struct* currentProcess)
{
    struct task_struct *task;
    struct task_struct *maxTask = currentProcess;
    struct list_head *temp;
    struct task_struct *taskTemp;
    if(list_empty(&currentProcess->children)){
        return maxTask;
    }
    list_for_each(temp, &currentProcess->children)
    {
        task = list_entry(temp, struct task_struct, sibling);
        taskTemp = get_heaviest_descendant(task);
        if(maxTask->weight < taskTemp->weight || (maxTask->weight == taskTemp->weight && maxTask->pid > taskTemp->pid))
        {
            maxTask = taskTemp;
        }
    }
    return maxTask;
}

asmlinkage long sys_get_heaviest_descendant(void)
{
    struct task_struct *task;
    struct task_struct *maxTask = 0;
    struct list_head *temp;
    struct task_struct *taskTemp;
    
    if(list_empty(&current->children)){
        return -ECHILD;
    }

    
    list_for_each(temp, &current->children)
    {
        task = list_entry(temp, struct task_struct, sibling);
        taskTemp = get_heaviest_descendant(task);
        if(maxTask == 0 || maxTask->weight < taskTemp->weight || (maxTask->weight == taskTemp->weight && maxTask->pid > taskTemp->pid))
        {
            maxTask = taskTemp;
        }
    }
    return maxTask->pid;
}




