#ifndef ISR_BOX_TEMP2_H_
#define ISR_BOX_TEMP2_H_

#define ISR_BOX_TEMP
#define ISR_BOX
#define ISR_TEMP

#define ISR_HARDWARE_TYPE			'B', 'o', 'x'
#define ISR_HARDWARE_NAME			'T', 'e', 'm', 'p'
#define ISR_HARDWARE_TYPE1		3
#define ISR_HARDWARE_TYPE2		50
#define ISR_HARDWARE_VERSION	1
#define ISR_SEGMENTS_COUNT		2

#ifndef BOOTLOADER
	#define PROGRAM_VERSION_MAJOR	1
	#define PROGRAM_VERSION_MINOR	1
	#define PROGRAM_YEAR					23
	#define PROGRAM_MONTH					2
	#define PROGRAM_DAY						18
#endif

#endif /* ISR_BOX_TEMP2_H_ */
