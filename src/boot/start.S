  	.code16
 	.text
	.global _start
	.extern boot_entry
_start:
	xor %ax,%ax
	mov %ax,%ds
	mov %ax,%ss
	mov %ax,%es
	mov %ax,%fs
	mov %ax,%gs /* initialize segment registers to 0 */

	mov $_start,%esp //0x7c00

read_loader:
	mov $0x8000,%bx /* the address of loader(es:[bx]) */
	mov $0x2,%ah
	mov $0x2,%cx /* cylinder 0, sector 2 */
	mov $64,%al /* the number of sector */
	mov $0x80,%dx /* specifiy the type of drive to hard disk */
	int $0x13 /* use BIOS interrupt(0x13) to read disk */
	jc read_loader /* if CF=1 jump to read_loader */
	jmp boot_entry

	jmp .

	.section boot_end, "ax"
boot_sig: .byte 0x55, 0xaa
