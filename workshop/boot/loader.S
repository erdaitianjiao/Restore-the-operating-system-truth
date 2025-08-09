%include "../include/boot.inc"

section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ  LOADER_BASE_ADDR
jmp loader_start


;create GDT and internal descriptor

    GDT_BASE:   dd 0x00000000
                dd 0x00000000

    CODE_DESC:  dd 0x0000FFFF
                dd DESC_CODE_HIGH4

    DATA_STACK_DESC:    dd 0x0000FFFF
                        dd DESC_DATA_HIGH4

    VIDEO_DESC:         dd 0x80000007             ;limit = (0xbffff - 0xb8000) / 4k = 0x7
                        dd DESC_VIDEO_HIGH4         ;now dpl is 0

    GDT_SIZE   equ $ - GDT_BASE
    GDT_LIMIT  equ GDT_SIZE - 1
    times 59   dq  0                            ;here to reserve 59 bits   
    times 5    db  0                            ;for whole number

    ;total_membyte used tp save capicatiy of mem
    ;the capicaty of loader is 0x200 
    ;loader.bin is loaded at 0x900
    ;so total_mem_byte is 0xb00
    total_mem_bytes dd 0x0000000

    SELECTOR_CODE  equ (0x0001 << 3) + TI_GDT + RPL0
    ;equal to (CODE_DESC - GDT_BASE)/8 + TI_GDT + RPL0

    SELECTOR_DATA  equ (0x0002 << 3) + TI_GDT + RPL0 

    SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0 

    ;below is gdt pointer ,first two bits is the range of GDT, last four bits is the start address of GDT

    gdt_ptr dw GDT_LIMIT
            dd GDT_BASE

    ;loadermsg db '2 loader in real.'
    
    ;total_mem_bytes4 + gdt_ptr6+ards_buf244 + ards_nr2 = 256
    ards_buf times 244 db 0
    ards_nr  dw 0                                   ;used to record number of struct ards

loader_start:

    xor ebx, ebx                                    ;set ebx to 20
    mov edx, 0x534d4150                                  
    mov di,  ards_buf                               ;set up buf

.e820_mem_get_loop:                                 ;loop get ards memory range struct
    mov eax, 0x0000e820                             ;revalue eax
    mov ecx, 20                                     ;ards address describe sizeof 20 bytes

    int 0x15
    jc .e820_failed_so_try_e801                     ;if cf equal 1 that error occured
    add di, cx                                      ;add di 20 to point next ards
    inc word [ards_nr]
    cmp eax, 0                                      ;if eax 0 and cf != 1 , got all the ards
    jnz .e820_mem_get_loop                         

    ;get max size of mem
    mov cx, [ards_nr]                               ;count
    mov ebx, ards_buf
    xor edx, edx                                    ;clear to zero

.find_max_mem_area:
    mov eax, [ebx]
    add eax, [ebx + 8]
    add ebx, 20
    cmp edx, eax
    jge .next_ards
    mov edx, eax

.next_ards:
    loop .find_max_mem_area
    jmp .mem_get_ok

;use 801 ,max get 4G mem
.e820_failed_so_try_e801:
    mov ax, 0xe801
    int 0x15
    jc .e801_failed_so_try_88
 
    ;calculate lower than 16 men
    mov cx, 0x400
    mul cx
    shl edx, 16
    and eax, 0x0000ffff
    or  edx, eax
    add edx, 0x100000
    mov esi, edx
    
    ;upper than 16m translate to byte
    xor eax, eax
    mov ax, bx
    mov ecx, 0x10000
    mul ecx
    mov edx, esi
    add edx, eax
    jmp .mem_get_ok

;use 88 max get 64mb
.e801_failed_so_try_88:
    mov ah, 0x88
    int 0x15
    jc .error_hlt
    and eax, 0x0000ffff
    mov cx, 0x400

    mul cx
    shl edx, 16
    or edx, eax
    add edx, 0x100000

.error_hlt:
    jmp $
.mem_get_ok:
    mov [total_mem_bytes], edx

;------------ready to get into protected mod------------;

    ;first step: switch on A20
    ;second step: load gdt
    ;third step: set cr0 pe to 1

    in al, 0x92
    or al, 0000_0010B
    out 0x92, al

    ;-----------------load GDT----------------------;
    
    lgdt [gdt_ptr]

    ;--------------set cr0 zero to 1------------;
    
    mov eax, cr0
    or  eax, 0x00000001
    mov cr0, eax
    
;-----------------create page-----------------;
    call setup_page

    ;reload 
    sgdt [gdt_ptr]
    
    ;mend gdt_ptr
    mov ebx, [gdt_ptr + 2]
    or dword [ebx + 0x18 + 4], 0xc0000000
    
    add dword [gdt_ptr + 2], 0xc0000000
    
    ;stack into kernel space
    add esp, 0xc0000000
    
    ;put page address to cr3
    mov eax, PAGE_DIR_TABLE_POS
    mov cr3, eax
    
    ;open cr0 31 bits
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    
    ;reload gdt
    lgdt [gdt_ptr]
    
    mov byte[gs: 160], "v"
    jmp $


    jmp dword SELECTOR_CODE: p_mode_start       ;reflash stream


;----------create directory and page table--------------;
setup_page:
    mov ecx, 4096                               ;clear the page space
    mov esi, 0

.clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS + esi], 0
    inc esi
    loop .clear_page_dir
;create pde(Page Directory Entry)
.create_pde:
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x10000                            ;point to the first pde
    mov ebx, eax

    ;put number 0 and number c00 the same pde every pde stand for 4mb
    ;this is prepared for kernel address
    or eax, PG_US_U | PG_RW_W | PG_P            ;set P 1, set US 1, every privaty have access to this
    mov [PAGE_DIR_TABLE_POS + 0x0]  , eax
    mov [PAGE_DIR_TABLE_POS + 0xc00], eax
    
    ;every page sizeof 4 bytes
    ;0xc00 stand 786 page, upper than 0xc00 is kernel space
    ;0xc0000000 - 0xffffffff is kernel space
    ;0x0 0xbfffffff is user space

    sub eax, 0x1000
    mov [PAGE_DIR_TABLE_POS + 4092], eax        ;make last page point to itself

    ;create PTE(Page Table Entry)
    mov ecx, 256
    mov esi, 0
    mov edx, PG_US_U | PG_RW_W | PG_P

.create_pte:
    ;ebx = 0x101000
    mov [ebx + esi *4], edx
    add edx, 4096
    inc esi
    loop .create_pte

;create other pde
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x2000                 ;now eax is at the second page table
    or eax, PG_US_U | PG_RW_W | PG_P
    mov ecx, 254
    mov esi, 769
.create_kernel_pde:
    mov [ebx + esi * 4], eax
    inc esi
    add eax, 0x1000
    loop .create_kernel_pde
    ret

[bits 32]
p_mode_start:
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, LOADER_STACK_TOP
    mov ax, SELECTOR_VIDEO
    mov gs, ax

    mov byte [gs: 160], 'P'

    jmp $