#include <SPI.h>
#include <RF24.h>

const unsigned int MAX_PAYLOAD_SIZE = 32;
const unsigned int MAX_NUMBER_PAYLOADS = 32;

// put function declarations here:
void readAndInterpretMessageFromSerial();
void sendMultiPayload();

// Variables
RF24 radio(7, 8); // (CE, CSN)

const byte address[6] = "1RF24"; // address / identifier

String messageToSend = "";
unsigned int numberOfPayloads = 0;
bool isSendingMPMG = false;
unsigned int payloadIndex = 0;

void setup()
{
  Serial.begin(115200);
  if (!radio.begin())
  {
    Serial.println("nRF24L01 module not connected!");
    while (1)
    {
    }
  }
  else
    Serial.println("nRF24L01 module connected!");

  /* Set the data rate:
   * RF24_250KBPS: 250 kbit per second
   * RF24_1MBPS:   1 megabit per second (default)
   * RF24_2MBPS:   2 megabit per second
   */
  radio.setDataRate(RF24_250KBPS);

  /* Set the power amplifier level rate:
   * RF24_PA_MIN:   -18 dBm
   * RF24_PA_LOW:   -12 dBm
   * RF24_PA_HIGH:   -6 dBm
   * RF24_PA_MAX:     0 dBm (default)
   */
  radio.setPALevel(RF24_PA_MAX); // sufficient for tests side by side

  /* Set the channel x with x = 0...125 => 2400 MHz + x MHz
   * Default: 76 => Frequency = 2476 MHz
   * use getChannel to query the channel
   */
  radio.setChannel(120);

  radio.openWritingPipe(address); // set the address
  radio.stopListening();          // set as transmitter

  /* You can choose if acknowlegdements shall be requested (true = default) or not (false) */
  radio.setAutoAck(true);

  /* with this you are able to choose if an acknowledgement is requested for
   * INDIVIDUAL messages.
   */
  radio.enableDynamicAck();

  /* setRetries(byte delay, byte count) sets the number of retries until the message is
   * successfully sent.
   * Delay time = 250 µs + delay * 250 µs. Default delay = 5 => 1500 µs. Max delay = 15.
   * Count: number of retries. Default = Max = 15.
   */
  radio.setRetries(5, 15);

  /* The default payload size is 32. You can set a fixed payload size which must be the
   * same on both the transmitter (TX) and receiver (RX)side. Alternatively, you can use
   * dynamic payloads, which need to be enabled on RX and TX.
   */
  // radio.setPayloadSize(11);
  radio.enableDynamicPayloads();
}

void loop()
{
  if (isSendingMPMG)
  {
    sendMultiPayload();
  }
  else
  {
    readAndInterpretMessageFromSerial();
  }
}

void readAndInterpretMessageFromSerial()
{
  while (Serial.available())
  {
    // Read incoming message from serial
    messageToSend = Serial.readString();
    // Check length pf message and decide, if it´s a multi payload message
    const unsigned int messageLength = messageToSend.length();
    if (messageLength > MAX_PAYLOAD_SIZE)
    {
      // calculate number of payloads
      numberOfPayloads = messageLength / MAX_PAYLOAD_SIZE + (messageLength % MAX_PAYLOAD_SIZE == 0 ? 0 : 1);
      if (numberOfPayloads > MAX_NUMBER_PAYLOADS)
      {
        Serial.print("Message too big - number of payloads = ");
        Serial.println(numberOfPayloads);
        return;
      }
      // Send MPMG command to receiver
      const String commandMessage = "MPMG" + String(numberOfPayloads < 9 ? "0" : "") + numberOfPayloads;
      if (radio.write(commandMessage.c_str(), commandMessage.length(), false))
      {
        isSendingMPMG = true;
        payloadIndex = 0;
        sendMultiPayload();
      }
      // Send single payload message
    }
    else if (messageLength > 0)
    {
      if (radio.write(messageToSend.c_str(), messageLength, false))
      { // false: acknowledgement request, true: no ack request
        Serial.println("Message successfully sent");
      }
    }
  }
}

void sendMultiPayload()
{
  // Get message slice
  unsigned int stringIndex = payloadIndex * MAX_PAYLOAD_SIZE;
  const String messageSlice = messageToSend.substring(stringIndex, stringIndex + MAX_PAYLOAD_SIZE);
  // Send payload
  if (messageSlice.length() > 0)
  {
    if (radio.write(messageSlice.c_str(), messageSlice.length(), false))
    { // false: acknowledgement request, true: no ack request
      String serialMessage = String("Message payload#") + String(payloadIndex) + String(" = ") + String(messageSlice) + String(" successfully sent\n");
      Serial.print(serialMessage);
    }
  }
  // Set index to next message
  payloadIndex++;
  // If it was the last message reset all variables
  if (payloadIndex >= numberOfPayloads)
  {
    messageToSend = "";
    numberOfPayloads = 0;
    isSendingMPMG = false;
    payloadIndex = 0;
  }
}