;------loader and kernel-------;
 
LOADER_BASE_ADDR    equ 0x900
LOADER_START_SECTOR equ 0x2


;-----gdt describe parameter------;

DESC_G_4K equ   1_00000000000000000000000b
DESC_D_32 equ   1_0000000000000000000000b
DESC_L    equ   0_000000000000000000000b

;64 bits code sign,now just need to sign to 0

DESC_AVL  equ   0_00000000000000000000b

;CPU dont touch this bit ,tempary set to 0

DESC_LIMIT_CODE2   equ  1111_0000000000000000b
DESC_LIMIT_DATA2   equ  DESC_LIMIT_CODE2
DESC_LIMIT_VIDEO2  equ  0000_000000000000000b
DESC_P             equ  1_000000000000000b
DESC_DPL_0         equ  00_0000000000000b
DESC_DPL_1         equ  01_0000000000000b
DESC_DPL_2         equ  10_0000000000000b
DESC_DPL_3         equ  11_0000000000000b
DESC_S_CODE        equ  1_000000000000b
DESC_S_DATA        equ  DESC_S_CODE
DESC_S_sys         equ  0_000000000000b

DESC_TYPE_CODE     equ  1000_00000000b
;x = 1, c = 0, r = 0, a = 0, code section is executable
DESC_TYPE_DATA     equ  0010_00000000b 
;x = 0, e = 0, w = 1, a = 0, data section is unexecutable


DESC_CODE_HIGH4   equ (0x00 << 24) + DESC_G_4K + DESC_D_32 +  DESC_L + DESC_AVL + DESC_LIMIT_CODE2  +  DESC_P+DESC_DPL_0 + DESC_S_CODE +DESC_TYPE_CODE + 0x00

DESC_DATA_HIGH4   equ (0x00 << 24) + DESC_G_4K + DESC_D_32 +  DESC_L + DESC_AVL + DESC_LIMIT_DATA2  +  DESC_P + DESC_DPL_0 + DESC_S_DATA +  DESC_TYPE_DATA + 0x00

DESC_VIDEO_HIGH4  equ (0x00 << 24) + DESC_G_4K + DESC_D_32 +  DESC_L + DESC_AVL + DESC_LIMIT_VIDEO2 + DESC_P +  DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x00

;choose vice parameter

RPL0   equ 00b
RPL1   equ 01b
RPL2   equ 10b
RPL3   equ 11b
TI_GDT equ 000b
TI_LDT equ 100b
