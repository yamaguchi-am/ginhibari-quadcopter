#include "communication.h"

#include "registers.h"

const int kTimeoutMillis = 300;

long last_receive_time;
int16_t reg[N_REGISTERS];

namespace {

int DecodeInt8(uint8_t* data) { return data[0]; }
int DecodeInt16(uint8_t* data) { return data[0] | (data[1] << 8); }

void EncodeInt16(int16_t value, uint8_t* data) {
  data[0] = value & 0xff;
  data[1] = value >> 8;
}

}  // namespace

bool CommTimedOut(long current_millis) {
  return current_millis > last_receive_time + kTimeoutMillis;
}

void OnUdpReceived(long current_millis, uint8_t data[], int length,
                   uint8_t retData[], int* retSize) {
  *retSize = 0;
  if (length < 2) return;
  int addr = data[1];
  if (addr < 0 || addr >= N_REGISTERS) {
    return;
  }
  int16_t value;
  int size;
  switch (data[0]) {
    case 0x0:  // read
      retData[0] = 0x0;
      (*retSize) += 1;
      EncodeInt16(reg[addr], &retData[1]);
      (*retSize) += 2;
      break;
    case 0x1:  // write
      value = DecodeInt16(&data[2]);
      reg[addr] = value;
      break;
    case 0x02:  // bulk read
      size = DecodeInt8(&data[2]);
      retData[(*retSize)++] = 0x2;
      retData[(*retSize)++] = addr;
      retData[(*retSize)++] = size;
      for (int i = 0; i < size; i++) {
        EncodeInt16(reg[addr + i], &retData[*retSize]);
        (*retSize) += 2;
      }
      break;
    case 0x03:  // bulk write
      size = DecodeInt8(&data[2]);
      for (int i = 0; i < size; i++) {
        reg[addr + i] = DecodeInt16(&data[3 + i * 2]);
      }
      break;
    default:
      Serial.write(data, length);
  }
  last_receive_time = millis();
}
