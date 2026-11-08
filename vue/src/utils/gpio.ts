export const MODE_BOARD_1 = {
  INPUT: 0,
  OUTPUT: 1,
  INPUT_PULLUP: 2,
  OUTPUT_OPEN_DRAIN: 3,
  PWM: 128,
  ONEWIRE: 129,
  ADC: 130,
  DAC: 131,
};
export const MODE_BOARD_2 = {
  INPUT: 1,
  OUTPUT: 3,
  INPUT_PULLUP: 5,
  OUTPUT_OPEN_DRAIN: 13,
  PWM: 128,
  ONEWIRE: 129,
  ADC: 130,
  DAC: 131,
};

export const COMMAND = {
  GPIO_COMMAND_SAVE: 254,
  GPIO_COMMAND_GET: 1,
  GPIO_COMMAND_SET: 2,
  GPIO_COMMAND_GET_ALL: 3,
  GPIO_COMMAND_CHANGE: 4,
  GPIO_COMMAND_RESTORE: 5,
};

export const LIST = {
  GPIO_BIT_INPUT: 1 << 0,
  GPIO_BIT_INPUT_PULLUP: 1 << 1,
  GPIO_BIT_OUTPUT: 1 << 2,
  GPIO_BIT_OUTPUT_OPEN_DRAIN: 1 << 3,
  GPIO_BIT_PWM: 1 << 4,
  GPIO_BIT_ONEWIRE: 1 << 5,
  GPIO_BIT_ADC: 1 << 6,
  GPIO_BIT_DAC: 1 << 7,
};

export const getBit = (byte: number, mask: number) => (byte & mask ? 1 : 0);
export const setBit = (byte: number, mask: number) => (byte |= mask);
export const clearBit = (byte: number, mask: number) => (byte &= ~mask);
export const toggleBit = (byte: number, mask: number) => (byte ^= mask);
