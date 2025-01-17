#ifndef KEYBOARD_H
#define KEYBOARD_H

// 这4颗键在键盘布局上的位置
#define KEY_FN_INDEX           70
#define KEY_REC_INDEX          72
#define KEY_CUSTOM_LEFT_INDEX  74
#define KEY_CUSTOM_RIGHT_INDEX 76
#define KEY_UPARROW_INDEX      75
#define KEY_DOWNARROW_INDEX    78
#define KEY_RIGHTARROW_INDEX   79
#define KEY_LEFTARROW_INDEX    77

// 这2颗键的数据放到移位寄存器的buffer中进行处理
#define ROCKER_KEY_X_INDEX 88
#define ROCKER_KEY_Y_INDEX 89

void keyboardStart(void);
uint8_t keyboardGetKeyState(uint8_t keyIndex, uint8_t bitIndex);

#endif // KEYBOARD_H