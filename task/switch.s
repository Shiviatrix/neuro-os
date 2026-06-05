.global task_switch_asm
task_switch_asm:
    push %ebp
    push %ebx
    push %esi
    push %edi

    mov 20(%esp), %eax
    mov %esp, (%eax)
    
    mov 24(%esp), %esp

    pop %edi
    pop %esi
    pop %ebx
    pop %ebp
    ret
