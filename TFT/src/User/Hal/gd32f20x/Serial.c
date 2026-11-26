#include "Serial.h"
#include "includes.h"

// set this line to "true" to enable IDLE Line interrupt. It is no more needed, so always set this macro to "false"
#define IDLE_LINE_IT false

DMA_CIRCULAR_BUFFER dmaL1DataRX[_UART_CNT] = {0};  // DMA / interrupt RX buffer
DMA_CIRCULAR_BUFFER dmaL1DataTX[_UART_CNT] = {0};  // DMA / interrupt TX buffer
SERIAL_RX_STATE serialRxState[_UART_CNT] = {0};

// config for USART DMA channels
const SERIAL_CFG Serial[_UART_CNT] = {
  #ifdef TX_DMA_WRITE
  // USART   RCU DMAx  DMAx  RX Channel TX Channel
    {USART0, RCU_DMA0, DMA0, 4,         3},
    {USART1, RCU_DMA0, DMA0, 5,         6},
    {USART2, RCU_DMA0, DMA0, 2,         1},
    {UART3,  RCU_DMA1, DMA1, 2,         4},
    {UART4,  RCU_DMA1, DMA1, 1,         3},
    {USART5, RCU_DMA1, DMA1, 5,         6},
  #else
  // USART   RCU DMAx  DMAx  RX Channel
    {USART0, RCU_DMA0, DMA0, 4},
    {USART1, RCU_DMA0, DMA0, 5},
    {USART2, RCU_DMA0, DMA0, 2},
    {UART3,  RCU_DMA1, DMA1, 2},
    {UART4,  RCU_DMA1, DMA1, 1},
    {USART5, RCU_DMA1, DMA1, 5},
  #endif
};

// disable RX DMA and clear all interrupt flags for a serial port
static void Serial_DMA_DisableAndClearFlagsRX(uint8_t port)
{
  const SERIAL_CFG * cfg = &Serial[port];

  // Disable DMA interrupt sources before stopping and clearing the channel.
  dma_interrupt_disable(cfg->dma_stream, cfg->dma_channelRX, DMA_INT_HTF);
  dma_interrupt_disable(cfg->dma_stream, cfg->dma_channelRX, DMA_INT_FTF);
  DMA_CHCTL(cfg->dma_stream, cfg->dma_channelRX) &= ~(1<<0);  // disable RX DMA

  // table 12.5.2, 4 bits per channel, shift = channel * 4

  switch (port)
  {
    case _USART1:
      DMA_INTC(DMA0) = (0x0F << 16);  // DMA0 channel 4
      break;

    case _USART2:
      DMA_INTC(DMA0) = (0x0F << 20);  // DMA0 channel 5
      break;

    case _USART3:
      DMA_INTC(DMA0) = (0x0F << 8);   // DMA0 channel 2
      break;

    case _UART4:
      DMA_INTC(DMA1) = (0x0F << 8);   // DMA1 channel 2
      break;

    case _UART5:
      DMA_INTC(DMA1) = (0x0F << 4);   // DMA1 channel 1
      break;

    case _USART6:
      DMA_INTC(DMA1) = (0x0F << 20);  // DMA1 channel 5
      break;
  }
}

#ifdef TX_DMA_WRITE

// disable TX DMA and clear all interrupt flags for a serial port
static void Serial_DMA_DisableAndClearFlagsTX(uint8_t port)
{
  DMA_CHCTL(Serial[port].dma_stream, Serial[port].dma_channelTX) &= ~(1<<0);  // disable TX DMA

  // table 12.5.2, 4 bits per channel, shift = channel * 4

  switch (port)
  {
    case _USART1:
      DMA_INTC(DMA0) = (0x0F << 12);  // DMA0 channel 3
      break;

    case _USART2:
      DMA_INTC(DMA0) = (0x0F << 24);  // DMA0 channel 6
      break;

    case _USART3:
      DMA_INTC(DMA0) = (0x0F << 4);   // DMA0 channel 1
      break;

    case _UART4:
      DMA_INTC(DMA1) = (0x0F << 16);  // DMA1 channel 4
      break;

    case _UART5:
      DMA_INTC(DMA1) = (0x0F << 12);  // DMA1 channel 3
      break;

    case _USART6:
      DMA_INTC(DMA1) = (0x0F << 24);  // DMA1 channel 6
      break;
  }
}

#endif

// Enable the DMA half/full transfer interrupts used to extend the circular
// DMA position into a monotonic byte counter.
static void Serial_DMA_RX_EnableIRQ(uint8_t port)
{
  const SERIAL_CFG * cfg = &Serial[port];

  dma_flag_clear(cfg->dma_stream, cfg->dma_channelRX, DMA_FLAG_HTF);
  dma_flag_clear(cfg->dma_stream, cfg->dma_channelRX, DMA_FLAG_FTF);
  dma_interrupt_enable(cfg->dma_stream, cfg->dma_channelRX, DMA_INT_HTF);
  dma_interrupt_enable(cfg->dma_stream, cfg->dma_channelRX, DMA_INT_FTF);

  // Each UART has its own DMA RX channel on GD32F20x.
  // Priority 2 keeps the DMA bookkeeping below the time-critical UART IRQs.
  switch (port)
  {
    case _USART1:
      NVIC_SetPriority(DMA0_Channel4_IRQn, 2U);
      NVIC_ClearPendingIRQ(DMA0_Channel4_IRQn);
      NVIC_EnableIRQ(DMA0_Channel4_IRQn);
      break;

    case _USART2:
      NVIC_SetPriority(DMA0_Channel5_IRQn, 2U);
      NVIC_ClearPendingIRQ(DMA0_Channel5_IRQn);
      NVIC_EnableIRQ(DMA0_Channel5_IRQn);
      break;

    case _USART3:
      NVIC_SetPriority(DMA0_Channel2_IRQn, 2U);
      NVIC_ClearPendingIRQ(DMA0_Channel2_IRQn);
      NVIC_EnableIRQ(DMA0_Channel2_IRQn);
      break;

    case _UART4:
      NVIC_SetPriority(DMA1_Channel2_IRQn, 2U);
      NVIC_ClearPendingIRQ(DMA1_Channel2_IRQn);
      NVIC_EnableIRQ(DMA1_Channel2_IRQn);
      break;

    case _UART5:
      NVIC_SetPriority(DMA1_Channel1_IRQn, 2U);
      NVIC_ClearPendingIRQ(DMA1_Channel1_IRQn);
      NVIC_EnableIRQ(DMA1_Channel1_IRQn);
      break;

    case _USART6:
      NVIC_SetPriority(DMA1_Channel5_IRQn, 2U);
      NVIC_ClearPendingIRQ(DMA1_Channel5_IRQn);
      NVIC_EnableIRQ(DMA1_Channel5_IRQn);
      break;
  }
}

// Consume pending DMA boundary events. HTF and FTF each represent half a
// buffer of received data. If both are pending together, servicing was delayed
// by at least one half-buffer interval; the protocol layer will resynchronise.
static void Serial_DMA_RX_ServiceEvents(uint8_t port)
{
  const SERIAL_CFG * cfg = &Serial[port];
  SERIAL_RX_STATE * state = &serialRxState[port];

  FlagStatus halfPending = dma_flag_get(cfg->dma_stream, cfg->dma_channelRX, DMA_FLAG_HTF);
  FlagStatus fullPending = dma_flag_get(cfg->dma_stream, cfg->dma_channelRX, DMA_FLAG_FTF);

  if ((halfPending == SET) && (fullPending == SET))
    state->coalescedEvents++;

  if (halfPending == SET)
  {
    dma_flag_clear(cfg->dma_stream, cfg->dma_channelRX, DMA_FLAG_HTF);
    state->halfTransferEvents++;
  }

  if (fullPending == SET)
  {
    dma_flag_clear(cfg->dma_stream, cfg->dma_channelRX, DMA_FLAG_FTF);
    state->halfTransferEvents++;
  }
}

// Return a monotonic producer count. The critical section prevents the DMA
// IRQ handler from updating the event count between the position and flag
// snapshots; DMA itself continues running.
uint32_t Serial_GetProducedCountRX(uint8_t port)
{
  if (port >= _UART_CNT)
    return 0;

  DMA_CIRCULAR_BUFFER * dma = &dmaL1DataRX[port];
  if (dma->cache == NULL || dma->cacheSize < 2)
    return 0;

  uint32_t primask = __get_PRIMASK();
  __disable_irq();

  const uint32_t cacheSize = dma->cacheSize;
  const uint32_t halfSize = cacheSize >> 1;
  if (halfSize == 0)
  {
    __set_PRIMASK(primask);
    return 0;
  }

  uint32_t position = 0;

  for (;;)
  {
    uint32_t remaining = DMA_CHCNT(Serial[port].dma_stream, Serial[port].dma_channelRX);

    // CNT may briefly be zero at the circular reload boundary. Normalise the
    // resulting position so it is always in [0, cacheSize).
    if (remaining <= cacheSize)
      position = (remaining == 0) ? 0 : cacheSize - remaining;
    else
      position = 0;

    FlagStatus halfPending = dma_flag_get(Serial[port].dma_stream, Serial[port].dma_channelRX, DMA_FLAG_HTF);
    FlagStatus fullPending = dma_flag_get(Serial[port].dma_stream, Serial[port].dma_channelRX, DMA_FLAG_FTF);

    if ((halfPending == RESET) && (fullPending == RESET))
      break;

    Serial_DMA_RX_ServiceEvents(port);
  }

  uint32_t completedHalfBuffers = serialRxState[port].halfTransferEvents;
  uint32_t produced = completedHalfBuffers * halfSize + (position % halfSize);

  dma->wIndex = position;
  __set_PRIMASK(primask);
  return produced;
}

static inline void Serial_DMA_Config(uint8_t port)
{
  const SERIAL_CFG * cfg = &Serial[port];

  rcu_periph_clock_enable(cfg->dma_rcc);                          // enable DMA clock

  Serial_DMA_DisableAndClearFlagsRX(port);                        // RX disable DMA and clear all interrupt flags

  DMA_CHPADDR(cfg->dma_stream, cfg->dma_channelRX) = (uint32_t)(&USART_DATA(cfg->uart));     // RX peripheral address (usart)
  DMA_CHMADDR(cfg->dma_stream, cfg->dma_channelRX) = (uint32_t)(dmaL1DataRX[port].cache);    // RX destination address (memory)
  DMA_CHCNT(cfg->dma_stream, cfg->dma_channelRX) = (uint32_t)(dmaL1DataRX[port].cacheSize);  // RX buffer size

  DMA_CHCTL(cfg->dma_stream, cfg->dma_channelRX) = 0;             // RX clear control register

  // primary serial port priority at highest level (TX higher than RX)
  if (port == SERIAL_PORT)
    DMA_CHCTL(cfg->dma_stream, cfg->dma_channelRX) |= (2<<12);    // RX priority level: High
  else
    DMA_CHCTL(cfg->dma_stream, cfg->dma_channelRX) |= (0<<12);    // RX priority level: Low

  //DMA_CHCTL(cfg->dma_stream, cfg->dma_channelRX) |= (0<<10);    // RX memory data size: 8 bit
  //DMA_CHCTL(cfg->dma_stream, cfg->dma_channelRX) |= (0<<8);     // RX peripheral data size: 8 bit
  DMA_CHCTL(cfg->dma_stream, cfg->dma_channelRX) |= (1<<7);       // RX memory increment mode
  //DMA_CHCTL(cfg->dma_stream, cfg->dma_channelRX) |= (0<<6);     // RX peripheral no increment mode
  DMA_CHCTL(cfg->dma_stream, cfg->dma_channelRX) |= (1<<5);       // RX circular mode enabled
  //DMA_CHCTL(cfg->dma_stream, cfg->dma_channelRX) |= (0<<4);     // RX data transfer direction: Peripheral-to-memory

  #ifdef TX_DMA_WRITE  // TX DMA based serial writing
    Serial_DMA_DisableAndClearFlagsTX(port);                      // TX disable DMA and clear all interrupt flags

    DMA_CHPADDR(cfg->dma_stream, cfg->dma_channelTX) = (uint32_t)(&USART_DATA(cfg->uart));  // TX peripheral address (usart)

    DMA_CHCTL(cfg->dma_stream, cfg->dma_channelTX) = 0;           // TX clear control register

    // primary serial port priority at highest level (TX higher than RX)
    if (port == SERIAL_PORT)
      DMA_CHCTL(cfg->dma_stream, cfg->dma_channelTX) |= (3<<12);  // TX priority level: Very high
    else
      DMA_CHCTL(cfg->dma_stream, cfg->dma_channelTX) |= (1<<12);  // TX priority level: Medium

    //DMA_CHCTL(cfg->dma_stream, cfg->dma_channelTX) |= (0<<10);  // TX memory data size: 8 bit
    //DMA_CHCTL(cfg->dma_stream, cfg->dma_channelTX) |= (0<<8);   // TX peripheral data size: 8 bit
    DMA_CHCTL(cfg->dma_stream, cfg->dma_channelTX) |= (1<<7);     // TX memory increment mode
    //DMA_CHCTL(cfg->dma_stream, cfg->dma_channelTX) |= (0<<6);   // TX peripheral no increment mode
    //DMA_CHCTL(cfg->dma_stream, cfg->dma_channelTX) |= (0<<5);   // TX circular mode disabled
    DMA_CHCTL(cfg->dma_stream, cfg->dma_channelTX) |= (1<<4);     // TX data transfer direction: Memory-to-Peripheral

    USART_CTL2(cfg->uart) |= (1<<7);                              // enable DMA transmitter (DMAT)
    //DMA_CHCTL(cfg->dma_stream, cfg->dma_channelTX) |= (1<<0);   // TX enable DMA, done later when needed
  #endif

  USART_CTL2(cfg->uart) |= (1<<6);                                // enable DMA receiver (DMAR)

  Serial_DMA_RX_EnableIRQ(port);
  DMA_CHCTL(cfg->dma_stream, cfg->dma_channelRX) |= (1<<0);       // RX enable DMA
}

static void Serial_ClearData(uint8_t port)
{
  memset(&serialRxState[port], 0, sizeof(serialRxState[port]));
  dmaL1DataRX[port].wIndex = dmaL1DataRX[port].rIndex = dmaL1DataRX[port].flag = dmaL1DataRX[port].timestamp = dmaL1DataRX[port].cacheSize = 0;

  if (dmaL1DataRX[port].cache != NULL)
  {
    free(dmaL1DataRX[port].cache);
    dmaL1DataRX[port].cache = NULL;
  }

  dmaL1DataTX[port].wIndex = dmaL1DataTX[port].rIndex = dmaL1DataTX[port].flag = dmaL1DataTX[port].timestamp = dmaL1DataTX[port].cacheSize = 0;

  if (dmaL1DataTX[port].cache != NULL)
  {
    free(dmaL1DataTX[port].cache);
    dmaL1DataTX[port].cache = NULL;
  }
}

void Serial_Config(uint8_t port, uint32_t cacheSizeRX, uint32_t cacheSizeTX, uint32_t baudrate)
{
  // Enable the DMA clock before inspecting or stopping a previously configured channel.
  rcu_periph_clock_enable(Serial[port].dma_rcc);

  // Stop an existing channel before releasing or replacing its DMA buffer.
  Serial_DMA_DisableAndClearFlagsRX(port);
  #ifdef TX_DMA_WRITE
    Serial_DMA_DisableAndClearFlagsTX(port);
  #endif
  Serial_ClearData(port);
  // The UART may be opened in the middle of a line. Require one LF boundary
  // before accepting the first protocol record after each (re)configuration.
  serialRxState[port].discardUntilLf = 1;

  dmaL1DataRX[port].cacheSize = cacheSizeRX;
  dmaL1DataRX[port].cache = malloc(cacheSizeRX);
  while (!dmaL1DataRX[port].cache);                           // RX malloc failed, blocking!

  dmaL1DataTX[port].cacheSize = cacheSizeTX;
  dmaL1DataTX[port].cache = malloc(cacheSizeTX);
  while (!dmaL1DataTX[port].cache);                           // TX malloc failed, blocking!

  UART_Config(port, baudrate, USART_INT_IDLE, IDLE_LINE_IT);  // configure serial line with or without IDLE Line interrupt

  Serial_DMA_Config(port);
}

void Serial_DeConfig(uint8_t port)
{
  // DeInit is also called for serial ports that may never have been enabled.
  rcu_periph_clock_enable(Serial[port].dma_rcc);

  // Stop DMA before freeing its target/source memory.
  Serial_DMA_DisableAndClearFlagsRX(port);

  #ifdef TX_DMA_WRITE
    Serial_DMA_DisableAndClearFlagsTX(port);
  #endif

  UART_DeConfig(port);
  Serial_ClearData(port);
}

#ifdef TX_DMA_WRITE  // TX DMA based serial writing

// DMA serial write support function
static void Serial_Send_TX(uint8_t port)
{
  // setup DMA transfer, and wait for serial Transfer Complete (TX) interrupt in ISR
  if (dmaL1DataTX[port].wIndex >= dmaL1DataTX[port].rIndex)
    dmaL1DataTX[port].flag = dmaL1DataTX[port].wIndex - dmaL1DataTX[port].rIndex;
  else  // split transfer into 2 parts
    dmaL1DataTX[port].flag = dmaL1DataTX[port].cacheSize - dmaL1DataTX[port].rIndex;

  Serial_DMA_DisableAndClearFlagsTX(port);   // disable TX DMA and clear all interrupt flags

  DMA_CHMADDR(Serial[port].dma_stream, Serial[port].dma_channelTX) = (uint32_t)(&dmaL1DataTX[port].cache[dmaL1DataTX[port].rIndex]);  // TX data start address
  DMA_CHCNT(Serial[port].dma_stream, Serial[port].dma_channelTX) = dmaL1DataTX[port].flag;                                            // number of bytes to transfer

  USART_CTL0(Serial[port].uart) |= USART_CTL0_TCIE;                          // enable Transfer Complete (TC) serial interrupt
  DMA_CHCTL(Serial[port].dma_stream, Serial[port].dma_channelTX) |= (1<<0);  // enable TX DMA
}

void Serial_Put(uint8_t port, const char * msg)
{
  dmaL1DataTX[port].timestamp = OS_GetTimeMs();  // keep track of last submitted message timestamp

  while (*msg)
  {
    // setup TX DMA, if no '\n' yet, but buffer is full AND DMA is not in progress already (waiting for Transfer Complete interrupt)
    if ((((dmaL1DataTX[port].wIndex + 1) % dmaL1DataTX[port].cacheSize) == dmaL1DataTX[port].rIndex) &&
        ((USART_CTL0(Serial[port].uart) & USART_CTL0_TCIE) == 0))
      Serial_Send_TX(port);

    // blocking! wait for buffer to become available
    while (((dmaL1DataTX[port].wIndex + 1) % dmaL1DataTX[port].cacheSize) == dmaL1DataTX[port].rIndex) { }

    dmaL1DataTX[port].cache[dmaL1DataTX[port].wIndex] = *msg;                                 // copy character to cache
    dmaL1DataTX[port].wIndex = (dmaL1DataTX[port].wIndex + 1) % dmaL1DataTX[port].cacheSize;  // update wIndex

    if ((*msg == '\n') && ((USART_CTL0(Serial[port].uart) & USART_CTL0_TCIE) == 0))
      Serial_Send_TX(port);  // start DMA process if message is complete and DMA is not in progress already

    msg++;  // let the compiler optimize this, no need to do it manually!
  }
}

#else  // TX interrupt based serial writing

void Serial_Put(uint8_t port, const char * msg)
{
  dmaL1DataTX[port].timestamp = OS_GetTimeMs();  // keep track of last submitted message timestamp

  while (*msg)
  {
    // blocking! wait for buffer to become available
    while (((dmaL1DataTX[port].wIndex + 1) % dmaL1DataTX[port].cacheSize) == dmaL1DataTX[port].rIndex) { };

    dmaL1DataTX[port].cache[dmaL1DataTX[port].wIndex] = *msg++;
    dmaL1DataTX[port].wIndex = (dmaL1DataTX[port].wIndex + 1) % dmaL1DataTX[port].cacheSize;

    USART_CTL0(Serial[port].uart) |= USART_CTL0_TBEIE;  // set TBE interrupt bit to start the serial transfer
  }
}

#endif

// DMA RX IRQ entry points. The handler only accounts for completed DMA
// half-buffers; byte parsing and line handling remain in the main context.
static void Serial_DMA_RX_IRQHandler(uint8_t port)
{
  Serial_DMA_RX_ServiceEvents(port);
}

void DMA0_Channel2_IRQHandler(void) { Serial_DMA_RX_IRQHandler(_USART3); }
void DMA0_Channel4_IRQHandler(void) { Serial_DMA_RX_IRQHandler(_USART1); }
void DMA0_Channel5_IRQHandler(void) { Serial_DMA_RX_IRQHandler(_USART2); }
void DMA1_Channel1_IRQHandler(void) { Serial_DMA_RX_IRQHandler(_UART5); }
void DMA1_Channel2_IRQHandler(void) { Serial_DMA_RX_IRQHandler(_UART4); }
void DMA1_Channel5_IRQHandler(void) { Serial_DMA_RX_IRQHandler(_USART6); }

// ISR, serial interrupt handler
void USART_IRQHandler(uint8_t port)
{
  #if IDLE_LINE_IT == true  // IDLE Line interrupt
    if ((USART_STAT0(Serial[port].uart) & USART_STAT0_IDLEF) != RESET)  // check for IDLE Line interrupt flag
    {
      dmaL1DataRX[port].wIndex = dmaL1DataRX[port].cacheSize - DMA_CHCNT(Serial[port].dma_stream, Serial[port].dma_channelRX);

      USART_DATA(Serial[port].uart);                                    // clear RXNE pending flag
      USART_STAT0(Serial[port].uart);                                   // as last, clear IDLE Line interrupt flag
    }
  #endif

  #ifdef TX_DMA_WRITE  // TX DMA based serial writing
    if ((USART_STAT0(Serial[port].uart) & USART_STAT0_TC) != RESET)             // check for Transfer Complete (TC) interrupt flag
    {
      // NOTE 1: use the serial TC, not the DMA TC because this only indicates DMA is done, peripheral might be still busy
      // NOTE 2: the TC interrupt is sometimes called while DMA is still active, so check NDTR status!
      //
      if (DMA_CHCNT(Serial[port].dma_stream, Serial[port].dma_channelTX) == 0)  // sending is complete
      {
        // NOTE: it marks message timestamp twice if transfer was split into 2 parts
        dmaL1DataTX[port].timestamp = OS_GetTimeMs();                           // keep track of last sent message timestamp

        dmaL1DataTX[port].rIndex = (dmaL1DataTX[port].rIndex + dmaL1DataTX[port].flag) % dmaL1DataTX[port].cacheSize;
        dmaL1DataTX[port].flag = 0;

        if (dmaL1DataTX[port].rIndex != dmaL1DataTX[port].wIndex)               // is more data available?
          Serial_Send_TX(port);                                                 // continue sending data
        else
          USART_CTL0(Serial[port].uart) &= ~USART_CTL0_TCIE;                    // disable Transfer Complete (TC) interrupt, nothing more to do
      }
      // else: more data is coming, wait for next Transfer Complete (TC) interrupt

      USART_STAT0(Serial[port].uart) &= ~USART_STAT0_TC;                        // as last, clear Transfer Complete (TC) interrupt flag
    }
  #else  // TX interrupt based serial writing
    if ((USART_STAT0(Serial[port].uart) & USART_STAT0_TBE) != RESET)                                  // check for TBE interrupt flag
    {
      if (dmaL1DataTX[port].rIndex != dmaL1DataTX[port].wIndex)                                       // is more data available?
      {
        if (dmaL1DataTX[port].cache[dmaL1DataTX[port].rIndex] == '\n')                                // is message complete?
          dmaL1DataTX[port].timestamp = OS_GetTimeMs();                                               // keep track of last sent message timestamp

        USART_DATA(Serial[port].uart) = (uint8_t) dmaL1DataTX[port].cache[dmaL1DataTX[port].rIndex];  // write next available character

        dmaL1DataTX[port].rIndex = (dmaL1DataTX[port].rIndex + 1) % dmaL1DataTX[port].cacheSize;      // increase reading index
      }
      else
      {
        USART_CTL0(Serial[port].uart) &= ~USART_CTL0_TBEIE;                                           // disable TXE interrupt, nothing more to do
      }
    }
  #endif
}

void USART0_IRQHandler(void)
{
  USART_IRQHandler(_USART1);
}

void USART1_IRQHandler(void)
{
  USART_IRQHandler(_USART2);
}

void USART2_IRQHandler(void)
{
  USART_IRQHandler(_USART3);
}

void UART3_IRQHandler(void)
{
  USART_IRQHandler(_UART4);
}

void UART4_IRQHandler(void)
{
  USART_IRQHandler(_UART5);
}

void USART5_IRQHandler(void)
{
  USART_IRQHandler(_USART6);
}
