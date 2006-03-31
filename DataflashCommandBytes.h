// Dataflash Opcodes:

#define FlashPageRead            0x52  // Main memory page read
#define FlashToBuf1Transfer      0x53  // Main memory page to buffer 1 transfer
#define Buf1Read                 0x54  // Buffer 1 read
#define FlashToBuf2Transfer      0x55  // Main memory page to buffer 2 transfer
#define Buf2Read                 0x56  // Buffer 2 read
#define StatusReg                0x57  // Status register
#define AutoPageReWrBuf1         0x58  // Auto page rewrite through buffer 1
#define AutoPageReWrBuf2         0x59  // Auto page rewrite through buffer 2
#define FlashToBuf1Compare       0x60  // Main memory page to buffer 1 compare
#define FlashToBuf2Compare       0x61  // Main memory page to buffer 2 compare
#define ContArrayRead            0x68  // Continuous Array Read (Note : Only A/B-parts supported)
#define PageErase                0x81  // Page erase, added by Martin Thomas
#define BlockErase               0x50  // Block erase, added by Dean Camera
#define FlashProgBuf1            0x82  // Main memory page program through buffer 1
#define Buf1ToFlashWE            0x83  // Buffer 1 to main memory page program with built-in erase
#define Buf1Write                0x84  // Buffer 1 write
#define FlashProgBuf2            0x85  // Main memory page program through buffer 2
#define Buf2ToFlashWE            0x86  // Buffer 2 to main memory page program with built-in erase
#define Buf2Write                0x87  // Buffer 2 write
#define Buf1ToFlash              0x88  // Buffer 1 to main memory page program without built-in erase
#define Buf2ToFlash              0x89  // Buffer 2 to main memory page program without built-in erase
