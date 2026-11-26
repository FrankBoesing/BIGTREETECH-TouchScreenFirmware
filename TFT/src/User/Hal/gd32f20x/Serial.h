#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <stdint.h>
#include "variants.h"  // for rcu_periph_enum etc.
#include "uart.h"      // for _UART_CNT etc.

// UART TX implementation selection.
// SERIAL_TX_DMA=1 uses DMA TX (default); SERIAL_TX_DMA=0 uses the
// interrupt-driven byte transmitter while keeping RX DMA enabled.
#ifndef SERIAL_TX_DMA
  #define SERIAL_TX_DMA 1
#endif

#if SERIAL_TX_DMA
  #define TX_DMA_WRITE
#endif

typedef struct
{
  char * cache;
  uint32_t cacheSize;
  volatile uint32_t wIndex;     // writing index
  volatile uint32_t rIndex;     // reading index
  volatile uint32_t flag;       // custom flag (for custom usage by the application)
  volatile uint32_t timestamp;  // keep track of message timestamp (currently used only by TX message queue)
} DMA_CIRCULAR_BUFFER;

// config for USART DMA channels
typedef struct
{
  uint32_t uart;
  rcu_periph_enum dma_rcc;  // uint32_t
  uint32_t dma_stream;
  uint32_t dma_channelRX;
  #ifdef TX_DMA_WRITE
    uint32_t dma_channelTX;
  #endif
} SERIAL_CFG;

typedef struct
{
  volatile uint32_t halfTransferEvents;  // Number of completed DMA half-buffers
  volatile uint32_t coalescedEvents;     // Both HTF and FTF pending when serviced
  uint32_t readCount;                    // Absolute byte count consumed by the protocol parser
  uint32_t scanCount;                    // Absolute byte count scanned for line endings
  uint32_t lineEndCount;                 // Absolute position of the LF for the pending line
  uint32_t observedCoalescedEvents;
  uint32_t overrunCount;
  uint32_t droppedLineCount;
  uint8_t lineReady;
  uint8_t discardUntilLf;
} SERIAL_RX_STATE;

extern DMA_CIRCULAR_BUFFER dmaL1DataRX[_UART_CNT];
extern DMA_CIRCULAR_BUFFER dmaL1DataTX[_UART_CNT];
extern const SERIAL_CFG Serial[_UART_CNT];
extern SERIAL_RX_STATE serialRxState[_UART_CNT];

void Serial_Config(uint8_t port, uint32_t cacheSizeRX, uint32_t cacheSizeTX, uint32_t baudrate);
void Serial_DeConfig(uint8_t port);

// Return the monotonic RX byte producer count for a GD32 circular DMA channel.
uint32_t Serial_GetProducedCountRX(uint8_t port);

// retrieve the next reading index in the RX message queue of the provided physical serial port:
//   - port: physical serial port
//
//   - return value: next reading index
static inline uint32_t Serial_GetReadingIndexRX(uint8_t port)
{
  return dmaL1DataRX[port].rIndex;
}

// retrieve the next writing index in the RX message queue of the provided physical serial port
// based on Interrupt/DMA status while writing serial data in the background:
//   - port: physical serial port
//
//   - return value: next writing index
static inline uint32_t Serial_GetWritingIndexRX(uint8_t port)
{
  uint32_t cacheSize = dmaL1DataRX[port].cacheSize;
  if (cacheSize == 0)
    return 0;

  uint32_t remaining = DMA_CHCNT(Serial[port].dma_stream, Serial[port].dma_channelRX);
  if (remaining >= cacheSize)
    return 0;

  return cacheSize - remaining;
}

// retrieve the last submitted or sent message timestamp in the TX message queue of the provided physical serial port
// based on Interrupt/DMA status while writing serial data in the background:
//   - port: physical serial port
//
//   - return value: last submitted or sent message timestamp
static inline uint32_t Serial_GetTimestampTX(uint8_t port)
{
  return dmaL1DataTX[port].timestamp;
}

// send a zero terminated message to UART port
//   - port: index of serial port
//   - msg: message to send
void Serial_Put(uint8_t port, const char * msg);

#endif
