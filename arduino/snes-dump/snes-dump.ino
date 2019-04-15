// SNES
//      back: GND A12 A13 A14 A15 BA0 BA1 BA2 BA3 BA4 BA5 BA6 BA7 CS  D4  D5  D6  D7  WE  --- --- --- Vcc
//     front: GND A11 A10 A09 A08 A07 A06 A05 A04 A03 A02 A01 A00 --- D0  D1  D2  D3  OE  --- --- RST Vcc
//
// Arduino Mega
//     ANALOG: A00-A7 A08-A15
//   register: PF0-7  PK0-7
//
//    DIGITAL: GND D52 D50 D48 D46 D44 D42 D40 D38 D36 D34 D32 D30 D28 D26 D24 D22 5V
//    DIGITAL: GND D53 D51 D49 D47 D45 D43 D41 D39 D37 D35 D33 D31 D29 D27 D25 D23 5V
//   register:     PB1 PB3 PL1 PL3 PL5 PL7 PG1 PD7 PC1 PC3 PC5 PC7 PA6 PA4 PA2 PA0
//   register:     PB0 PB2 PL0 PL2 PL4 PL6 PG0 PG2 PC0 PC2 PC4 PC6 PA7 PA5 PA3 PA1
//
// Pin Mapping
//       SNES: A00-A07 A08-A15 BA0-BA7 D0-D7 CS OE WE  RST
//    Arduino: D22-D29 D37-D30 D49-D42 A0-A7 A8 A9 A10 A11
//   register: physical_address = (PORTL << 16) | (PORTC << 8) | (PORTA)
//             data             = PORTF
//             RST,WE,OE,CS     = PORTK
//
// LoROM
//   physical_address = (logical_address[22:15] << 16) | (1 << 15) | (logical_address[14:0])
// HiRom
//   physical_address = logical_address

char g_Title[22];
bool g_IsHiRom = false;
uint8_t g_RomStruct = 0;
uint16_t g_RomSizeKB = 0;

enum RomStruct {
  kRomOnly    = 0x00,
  kRomAndRam  = 0x01,
  kRomAndSram = 0x02,
};
enum Flags {
  kChipEnable   = 0x01,
  kOutputEnable = 0x02,
  kWriteEnable  = 0x04,
  kReset        = 0x08,
};

inline void Wait()
{
  // nop x 16 = 1000ns
  __asm__ volatile(
    R"asm(
      nop
      nop
      nop
      nop
      nop
      nop
      nop
      nop
      nop
      nop
      nop
      nop
      nop
      nop
      nop
      nop
    )asm"
  );
}

//! Read HiROM data
inline uint8_t ReadData(uint32_t address)
{
  PORTL = (address >> 16) & 0xFF;
  PORTC = (address >>  8) & 0xFF;
  PORTA = (address      ) & 0xFF;

  Wait();

  return PINF;
}

//! Read LoROM data
inline uint8_t ReadLoRomData(uint32_t address)
{
  // address = 
  PORTL = (address >> 15) & 0xFF;
  PORTC = (address >>  8) & 0xFF | 0x80;
  PORTA = (address      ) & 0xFF;

  Wait();

  return PINF;
}

//! RST,WE,OE,CS
void SetFlags(uint8_t flags)
{
  PORTK = flags;
  Wait();
}

void ReadRomInfo()
{
  const uint32_t kAddr = 0x0000FFC0L;

  SetFlags(0);

  for ( uint32_t i = 0; i < 21; ++i ) {
    g_Title[i] = ReadData(kAddr + i);
  }
  g_Title[21] = '\0';

  g_IsHiRom = bool(ReadData(kAddr + 21) & 0x01);
  g_RomStruct = ReadData(kAddr + 22);
  g_RomSizeKB = 1 << ReadData(kAddr + 23);
}

void PrintRomInfo()
{
  Serial.println(g_Title);

  if ( g_IsHiRom ) {
    Serial.print("HiROM\n");
  } else {
    Serial.print("LoRom\n");
  }

  switch ( g_RomStruct ) {
    case kRomOnly:    Serial.print("ROM\n"); break;
    case kRomAndRam:  Serial.print("ROM+RAM\n"); break;
    case kRomAndSram: Serial.print("ROM+SRAM\n"); break;
  }

  Serial.print(uint16_t(g_RomSizeKB));
  Serial.print("KB\n");
}

void DumpRom()
{
  uint8_t data[1024];

  if ( g_IsHiRom ) {
    for ( uint32_t addressKB = 0; addressKB < g_RomSizeKB; ++addressKB ) {
      for ( uint32_t i = 0; i < 1024; ++i ) {
        data[i] = ReadData((addressKB << 10) | i);
      }
      Serial.write(data, sizeof(data));
    }
  } else {
    for ( uint32_t addressKB = 0; addressKB < g_RomSizeKB; ++addressKB ) {
      for ( uint32_t i = 0; i < 1024; ++i ) {
        data[i] = ReadLoRomData((addressKB << 10) | i);
      }
      Serial.write(data, sizeof(data));
    }
  }
}

void setup()
{
  Serial.begin(230400);

  Serial.println("Hello");

  // disable timer interruption
  TIMSK0 = 0;

  // address
  DDRA  = 0b11111111;
  DDRC  = 0b11111111;
  DDRL  = 0b11111111;
  // data
  DDRF  = 0b11111111;
  PORTF = 0b11111111;
  DDRF  = 0b00000000;
  // flags
  DDRK |= 0b00001111;
  Wait();

  // reset
  SetFlags(kReset | kWriteEnable);

  ReadRomInfo();

  // handshake
  while ( -1 == Serial.read() ) {}

  // send rom size
  Serial.write(g_RomSizeKB >> 8);
  Serial.write(g_RomSizeKB & 0x00FF);

  // dump
  //PrintRomInfo();
  DumpRom();
}

void loop()
{
  // put your main code here, to run repeatedly:

}
