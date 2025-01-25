#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/list.h>


asmlinkage long sys_hello(void) {
    printk("Hello, World!\n");
    return 0;
}

asmlinkage long sys_set_weight(int weight) {
    if(weight >=0){
        current->weight  = weight;
        return 0;
    }
    return -EINVAL;
}

asmlinkage long sys_get_weight(void) {
    return current->weight;
}

asmlinkage long sys_get_siblings_sum(void) {
        int sum;
        int count;
        struct task_struct *sib;
        struct task_struct *p;
        struct list_head *list;

        p = current->parent;
        sum = 0;
        count = 0;

        list_for_each(list, &p->children){
            count++;
        }

        if(count <= 1){
            return -ESRCH;
        }

        list_for_each(list, &p->children){
            sib = list_entry(list,struct task_struct,sibling);
            if(sib->pid != current->pid){
                sum+= sib->weight;
            }
        } 
        return sum;
}

asmlinkage long sys_get_lightest_divisor_ancestor(void) {
    struct task_struct *p;
    int org_weight;
    int min_weight;
    int min_pid;
    int weight;

    p = current;
    org_weight = p->weight;
    min_weight = org_weight;
    min_pid = p->pid;

    if(org_weight == 0){
        return min_pid;
    }
    while(p->pid !=1){
        p = p->parent;
        weight = p->weight; 
        if(weight != 0 && org_weight % weight == 0 && min_weight > weight){
            min_pid = p->pid;
            min_weight = weight;
        }
    }
    return min_pid;
}
