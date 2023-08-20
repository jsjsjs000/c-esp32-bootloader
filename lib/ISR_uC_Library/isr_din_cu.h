#ifndef ISR_DIN_CU_H_
#define ISR_DIN_CU_H_

#define ISR_DIN_CU
#define ISR_DIN
#define ISR_CU

#define ISR_HARDWARE_TYPE			'D', 'i', 'n'
#define ISR_HARDWARE_NAME			'C', 'U'
#define ISR_HARDWARE_TYPE1		2
#define ISR_HARDWARE_TYPE2		1
#define ISR_HARDWARE_VERSION	1
#define ISR_SEGMENTS_COUNT		1

#ifndef BOOTLOADER
	#define PROGRAM_VERSION_MAJOR	1
	#define PROGRAM_VERSION_MINOR	2
	#define PROGRAM_YEAR					23
	#define PROGRAM_MONTH					3
	#define PROGRAM_DAY						15
#endif

#endif /* ISR_DIN_CU_H_ */
