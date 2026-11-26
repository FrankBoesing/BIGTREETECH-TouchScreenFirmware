#include "SerialConnection.h"
#include "includes.h"

#define SERIAL_PORT_RX_QUEUE_SIZE   NOBEYOND(256, RAM_SIZE * 64, 4096)  // ACK messages reading queue from mainboard
#define SERIAL_PORT_2_RX_QUEUE_SIZE 512
#define SERIAL_PORT_3_RX_QUEUE_SIZE 512
#define SERIAL_PORT_4_RX_QUEUE_SIZE 512

#define SERIAL_PORT_TX_QUEUE_SIZE   256  // gcodes writing queue to mainboard
#define SERIAL_PORT_2_TX_QUEUE_SIZE 256
#define SERIAL_PORT_3_TX_QUEUE_SIZE 256
#define SERIAL_PORT_4_TX_QUEUE_SIZE 256

const SERIAL_PORT_INFO serialPort[SERIAL_PORT_COUNT] = {
  {SERIAL_PORT,     SERIAL_PORT_RX_QUEUE_SIZE,   SERIAL_PORT_TX_QUEUE_SIZE,   "",  "1 - Printer"},
  #ifdef SERIAL_PORT_2
    {SERIAL_PORT_2, SERIAL_PORT_2_RX_QUEUE_SIZE, SERIAL_PORT_2_TX_QUEUE_SIZE, "2", "2 - WiFi"},
  #endif
  #ifdef SERIAL_PORT_3
    {SERIAL_PORT_3, SERIAL_PORT_3_RX_QUEUE_SIZE, SERIAL_PORT_3_TX_QUEUE_SIZE, "3", "3 - UART3"},
  #endif
  #ifdef SERIAL_PORT_4
    {SERIAL_PORT_4, SERIAL_PORT_4_RX_QUEUE_SIZE, SERIAL_PORT_4_TX_QUEUE_SIZE, "4", "4 - UART4"}
  #endif
};

const uint32_t baudrateValues[BAUDRATE_COUNT] = {
  0,     2400,   9600,   19200,   38400,   57600,   115200,   230400,   250000,   500000,   921600,   1000000};

const char * const baudrateNames[BAUDRATE_COUNT] = {
  "OFF", "2400", "9600", "19200", "38400", "57600", "115200", "230400", "250000", "500000", "921600", "1000000"};

void Serial_Init(SERIAL_PORT_INDEX portIndex)
{
  if (!WITHIN(portIndex, ALL_PORTS, SERIAL_PORT_COUNT - 1))
    return;

  if (portIndex == PORT_1 || portIndex == ALL_PORTS)  // if primary or all serial ports, initialize the primary serial port first
  {
    InfoHost_Init(false);  // initialize infoHost when disconnected

    Serial_Config(serialPort[PORT_1].port, serialPort[PORT_1].cacheSizeRX, serialPort[PORT_1].cacheSizeTX, baudrateValues[infoSettings.serial_port[PORT_1]]);
  }

  #ifdef SERIAL_PORT_2
    // if ALL_PORTS, SUP_PORTS or specific supplementary port
    //
    for (SERIAL_PORT_INDEX tmpIndex = PORT_2; tmpIndex < SERIAL_PORT_COUNT; tmpIndex++)
    {
      // the supplementary serial ports should be enabled according to config.ini.
      // Disable the serial port when it is not in use and/or not connected to a device (floating) to
      // avoid to receive and process wrong data due to possible electromagnetic interference (EMI)

      // if serial port is enabled...
      if (infoSettings.serial_port[tmpIndex] > 0 && (portIndex == tmpIndex || portIndex < PORT_1))
        Serial_Config(serialPort[tmpIndex].port, serialPort[tmpIndex].cacheSizeRX, serialPort[tmpIndex].cacheSizeTX,
                      baudrateValues[infoSettings.serial_port[tmpIndex]]);
    }
  #endif
}

void Serial_DeInit(SERIAL_PORT_INDEX portIndex)
{
  if (!WITHIN(portIndex, ALL_PORTS, SERIAL_PORT_COUNT - 1))
    return;

  if (portIndex == PORT_1 || portIndex == ALL_PORTS)  // if primary or all serial ports, deinitialize the primary serial port first
    Serial_DeConfig(serialPort[PORT_1].port);

  #ifdef SERIAL_PORT_2
    // if ALL_PORTS, SUP_PORTS or specific supplementary port
    //
    for (SERIAL_PORT_INDEX tmpIndex = PORT_2; tmpIndex < SERIAL_PORT_COUNT; tmpIndex++)
    {
      if (portIndex == tmpIndex || portIndex < PORT_1)
        Serial_DeConfig(serialPort[tmpIndex].port);
    }
  #endif
}

void Serial_Forward(SERIAL_PORT_INDEX portIndex, const char * msg)
{
  if (!WITHIN(portIndex, ALL_PORTS, SERIAL_PORT_COUNT - 1))
    return;

  #if defined(SERIAL_DEBUG_PORT) && defined(DEBUG_SERIAL_COMM)
    // dump serial data sent to debug port
    Serial_Put(SERIAL_DEBUG_PORT, ">>");
    Serial_Put(SERIAL_DEBUG_PORT, msg);
  #endif

  uint8_t portCount = SERIAL_PORT_COUNT;  // by default, select all the serial ports

  if (portIndex == ALL_PORTS)         // if ALL_PORTS, forward the message to all the enabled serial ports
    portIndex = PORT_1;
  #ifdef SERIAL_PORT_2
    else if (portIndex == SUP_PORTS)  // if SUP_PORTS, forward the message to all the enabled supplementary serial ports
      portIndex = PORT_2;
  #endif
  else                                // if specific port, forward the message only to that specific serial port, if enabled
    portCount = portIndex + 1;

  for (; portIndex < portCount; portIndex++)
  {
    // forward data only if serial port is enabled
    if (infoSettings.serial_port[portIndex] > 0
      #ifdef SERIAL_DEBUG_PORT
        && serialPort[portIndex].port != SERIAL_DEBUG_PORT  // do not forward data to serial debug port
      #endif
      )
      Serial_Put(serialPort[portIndex].port, msg);  // pass on the message to the port
  }
}

#if defined(GD32F2XX)

#define SERIAL_RX_MAX_LINE_LENGTH 511U

static void Serial_RX_Resync(uint8_t port, uint32_t produced, bool overrun)
{
  DMA_CIRCULAR_BUFFER * dma = &dmaL1DataRX[port];
  SERIAL_RX_STATE * state = &serialRxState[port];

  if (overrun)
    state->overrunCount++;

  // Throw away the uncertain fragment and wait for the next line boundary.
  state->readCount = produced;
  state->scanCount = produced;
  state->lineReady = 0;
  state->discardUntilLf = 1;
  dma->rIndex = produced % dma->cacheSize;
  dma->flag = dma->rIndex;
  dma->wIndex = dma->rIndex;
}

static void Serial_RX_ConsumeLine(uint8_t port)
{
  DMA_CIRCULAR_BUFFER * dma = &dmaL1DataRX[port];
  SERIAL_RX_STATE * state = &serialRxState[port];

  state->readCount = state->lineEndCount + 1U;
  state->scanCount = state->readCount;
  state->lineReady = 0;

  dma->rIndex = state->readCount % dma->cacheSize;
  dma->flag = dma->rIndex;
}

bool Serial_DataAvailableRX(uint8_t port)
{
  DMA_CIRCULAR_BUFFER * dma = &dmaL1DataRX[port];
  SERIAL_RX_STATE * state = &serialRxState[port];

  if (dma->cache == NULL || dma->cacheSize < 2U)
    return false;

  const uint32_t cacheSize = dma->cacheSize;
  uint32_t produced = Serial_GetProducedCountRX(port);
  dma->wIndex = produced % cacheSize;

  // If both DMA boundary flags were pending together, interrupt service was
  // delayed by at least half a DMA buffer. Do not parse any fragment spanning
  // that uncertain interval.
  if (state->observedCoalescedEvents != state->coalescedEvents)
  {
    state->observedCoalescedEvents = state->coalescedEvents;
    state->droppedLineCount++;
    Serial_RX_Resync(port, produced, false);
    return false;
  }

  if (state->lineReady)
  {
    if ((uint32_t)(produced - state->readCount) > cacheSize)
    {
      Serial_RX_Resync(port, produced, true);
      return false;
    }

    return true;
  }

  // Unsigned subtraction is intentional: the monotonic 32-bit byte counter
  // wraps naturally, while the retained window is always much smaller.
  if ((uint32_t)(produced - state->readCount) > cacheSize)
  {
    Serial_RX_Resync(port, produced, true);
    return false;
  }

  while (state->scanCount != produced)
  {
    if ((uint32_t)(produced - state->scanCount) > cacheSize)
    {
      Serial_RX_Resync(port, produced, true);
      return false;
    }

    char value = dma->cache[state->scanCount % cacheSize];
    state->scanCount++;

    if (state->discardUntilLf)
    {
      state->readCount = state->scanCount;

      if (value == '\n')
        state->discardUntilLf = 0;

      continue;
    }

    if (value == '\n')
    {
      state->lineEndCount = state->scanCount - 1U;
      state->lineReady = 1;
      dma->flag = state->lineEndCount % cacheSize;
      return true;
    }

    if ((uint32_t)(state->scanCount - state->readCount) >= SERIAL_RX_MAX_LINE_LENGTH)
    {
      // The line cannot fit into the ACK/command receive buffers. Discard
      // it through LF instead of passing a truncated line to a parser.
      state->droppedLineCount++;
      state->readCount = state->scanCount;
      state->discardUntilLf = 1;
    }
  }

  return false;
}

uint16_t Serial_Get(uint8_t port, char * buf, uint16_t bufSize)
{
  DMA_CIRCULAR_BUFFER * dma = &dmaL1DataRX[port];
  SERIAL_RX_STATE * state = &serialRxState[port];

  if (buf == NULL || bufSize == 0 || !state->lineReady)
    return 0;

  const uint32_t cacheSize = dma->cacheSize;
  uint32_t produced = Serial_GetProducedCountRX(port);
  dma->wIndex = produced % cacheSize;

  if (state->observedCoalescedEvents != state->coalescedEvents)
  {
    state->observedCoalescedEvents = state->coalescedEvents;
    state->droppedLineCount++;
    Serial_RX_Resync(port, produced, false);
    return 0;
  }

  if ((uint32_t)(produced - state->readCount) > cacheSize)
  {
    Serial_RX_Resync(port, produced, true);
    return 0;
  }

  uint32_t lineLength = state->lineEndCount - state->readCount + 1U;
  if (lineLength == 0 || lineLength > cacheSize)
  {
    state->droppedLineCount++;
    Serial_RX_ConsumeLine(port);
    return 0;
  }

  uint32_t leadingSpaces = 0;
  while (leadingSpaces < (lineLength - 1U) &&
         dma->cache[(state->readCount + leadingSpaces) % cacheSize] == ' ')
    leadingSpaces++;

  uint32_t start = state->readCount + leadingSpaces;
  uint32_t copyLength = lineLength - leadingSpaces;
  if ((copyLength + 1U) > bufSize)
  {
    // The caller's destination is smaller than this complete line. Consume
    // and discard it; never leave a permanently blocking pending line.
    state->droppedLineCount++;
    Serial_RX_ConsumeLine(port);
    return 0;
  }

  for (uint32_t i = 0; i < copyLength; i++)
    buf[i] = dma->cache[(start + i) % cacheSize];

  buf[copyLength] = '\\0';
  Serial_RX_ConsumeLine(port);

  return (uint16_t)copyLength;
}

#else  // legacy RX path for non-GD32 targets

bool Serial_DataAvailableRX(uint8_t port)
{
  // NOTE: used 32 bit variables for performance reasons

  // wIndex: update L1 cache's writing index (dynamically changed (by IDLE interrupt handler) variables/attributes)
  //         and make a static access (32 bit) to it to speedup performance on this function
  //
  uint32_t wIndex = dmaL1DataRX[port].wIndex = Serial_GetWritingIndexRX(port);
  uint32_t flag = dmaL1DataRX[port].flag;

  if (flag == wIndex)
    return 0;

  uint32_t cacheSize = dmaL1DataRX[port].cacheSize;

  while (dmaL1DataRX[port].cache[flag] != '\n' && flag != wIndex)
    flag = (flag + 1) % cacheSize;

  dmaL1DataRX[port].flag = flag;
  return (flag != wIndex);
}

uint16_t Serial_Get(uint8_t port, char * buf, uint16_t bufSize)
{
  // NOTE: used 32 bit variables for performance reasons (in particular for data copy)
  DMA_CIRCULAR_BUFFER * dmaL1Data_ptr = &dmaL1DataRX[port];
  char * cache = dmaL1Data_ptr->cache;
  uint32_t cacheSize = dmaL1Data_ptr->cacheSize;
  uint32_t rIndex = dmaL1Data_ptr->rIndex;
  uint32_t flag = dmaL1Data_ptr->flag;

  while (cache[rIndex] == ' ' && rIndex != flag)
    rIndex = (rIndex + 1) % cacheSize;

  uint32_t msgSize = (cacheSize + flag - rIndex) % cacheSize + 1;

  if (bufSize < (msgSize + 1))
    return 0;

  if (rIndex <= flag)
  {
    while (rIndex <= flag)
      *(buf++) = cache[rIndex++];
  }
  else
  {
    while (rIndex < cacheSize)
      *(buf++) = cache[rIndex++];

    rIndex = 0;

    while (rIndex <= flag)
      *(buf++) = cache[rIndex++];
  }

  *buf = '\\0';
  dmaL1Data_ptr->flag = dmaL1Data_ptr->rIndex = (flag + 1) % cacheSize;
  return msgSize;
}

#endif  // GD32F2XX RX implementation

#ifdef SERIAL_PORT_2

void Serial_GetFromUART(void)
{
  CMD cmd;

  // scan all the supplementary serial ports
  for (SERIAL_PORT_INDEX portIndex = PORT_2; portIndex < SERIAL_PORT_COUNT; portIndex++)
  {
    // retrieve data only if serial port is enabled
    if (infoSettings.serial_port[portIndex] > 0
      #ifdef SERIAL_DEBUG_PORT
        && serialPort[portIndex].port != SERIAL_DEBUG_PORT  // do not forward data to serial debug port
      #endif
      )
    {
      while (Serial_DataAvailableRX(serialPort[portIndex].port) && Serial_Get(serialPort[portIndex].port, cmd, CMD_MAX_SIZE) != 0)
      {
        handleCmd(cmd, portIndex);
      }
    }
  }
}

#endif
