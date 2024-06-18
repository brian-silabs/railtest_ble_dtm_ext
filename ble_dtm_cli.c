#include "sl_cli.h"
#include "sl_cli_config.h"
#include "sl_cli_command.h"
#include "sl_cli_arguments.h"

#include "response_print.h"
#include "app_common.h"
#include "sl_cli_handles.h"

#define   DTM_HEADER_LENGTH_BYTES     2
#define   PRBS9_LENGTH                511
#define   BYTE_LENGTH                 8

/* LE Test packet PDU type headers - payload shown as big endian (not transmission order) */
#define   PDU_LETEST_PRBS9            0x00
#define   PDU_LETEST_0FH              0x01
#define   PDU_LETEST_55H              0x02
#define   PDU_LETEST_PRBS15           0x03
#define   PDU_LETEST_FFH              0x04
#define   PDU_LETEST_00H              0x05
#define   PDU_LETEST_F0H              0x06
#define   PDU_LETEST_AAH              0x07


static bool generate_prbs9(uint8_t *prbs9_sequence, uint8_t length)
{
    uint16_t state = 0x1FF;  // Initial state with 9 bits set to 1
    uint16_t lenght_bits = ((length * 8) - 1);
    uint16_t i = 0;

    // Check we are not expecting more than authorized by BLE spec
    if(lenght_bits > PRBS9_LENGTH)
    {
      return false;
    }

    // Initialize RAIL Payload to all 0x00s as we will perform bitwise masked operations
    for (i = 0; i < length; ++i) {
      prbs9_sequence[i] = 0;
    }

    // Compute state to generate prbs9 sequence
    for (i = 0; i < lenght_bits; i++) {
        // The new bit is the XOR of bit 0 and bit 4 (counting from 0)
        uint8_t new_bit = ((state >> 0) & 1) ^ ((state >> 4) & 1);

        //byteIndex contains the current byte we are performing ops on
        uint8_t byteIndex = i / BYTE_LENGTH;

        //bitshift contains the right shift we need to perform to save our computed bit
        uint8_t bitShift = (BYTE_LENGTH - 1) - (i % BYTE_LENGTH);

        //We store (state & 1) at the right index, right shifted according to above
        prbs9_sequence[byteIndex] |= ((state & 1) << bitShift);

        //We update state, to reiterate
        state = (state >> 1) | (new_bit << 8);  // Shift right and insert new bit at MSB
    }

    return true;
}

static void generate_pattern(uint8_t *pattern_sequence, uint8_t pattern, uint8_t length)
{
  for (uint8_t i = 0; i < length; i++) {
    pattern_sequence[i] = pattern;  // Store the least significant bit
  }
}

/*
 * The sl_cli_get_argument_count() can include the start index then
 * payload. Remove 1 from the count when checking "> sizeof(txData)" to
 * compensate for the index and leave just the size of the payload.
 * The test test_payloadMultipleFifoSize checks for
 * exactly MAX_PAYLOAD = 1024 and if the offset count is included the
 * check is throws the check off by 1.
 * Specifically, the test being run is:
 *  "setTxPayloadQuiet 1008 240 241 242 243 244 245 246 247 248 249 250 251 252 253 254 255"
 * which results in offset = 1008 and count = 17 so 1 must be removed
 * from the count to accommodate the precise MAX_PAYLOAD = 1024.
 * Reducing the count during the check in this way does not affect
 * any other tests.
 */
static bool setDtmTxPayloadHelper(sl_cli_command_arg_t *args)
{
  uint8_t packetType = sl_cli_get_argument_uint8(args, 0);
  // Make sure this fits in the buffer
  uint8_t count = (uint8_t)sl_cli_get_argument_uint8(args, 1);

  if(SL_RAIL_TEST_MAX_PACKET_LENGTH < (count + DTM_HEADER_LENGTH_BYTES))
  {
    responsePrintError(sl_cli_get_command_string(args, 0), 5, "Data overflows txData buffer");
    return false;
  }

  txDataLen = count + DTM_HEADER_LENGTH_BYTES;

  switch (packetType) {
    case PDU_LETEST_PRBS9:
      if(!generate_prbs9(&txData[2], count))
      {
        responsePrintError(sl_cli_get_command_string(args, 0), 5, "Failed to generate PRBS9 payload");
        return false;
      }
      break;
    case PDU_LETEST_0FH :
      generate_pattern(&txData[2], 0x0F, count);
      break;
    case PDU_LETEST_55H :
      generate_pattern(&txData[2], 0x55, count);
      break;
    case PDU_LETEST_FFH :
      generate_pattern(&txData[2], 0xFF, count);
      break;
    case PDU_LETEST_00H :
      generate_pattern(&txData[2], 0x00, count);
      break;
    case PDU_LETEST_F0H :
      generate_pattern(&txData[2], 0xF0, count);
      break;
    case PDU_LETEST_AAH :
      generate_pattern(&txData[2], 0xAA, count);
      break;
    default:
      responsePrintError(sl_cli_get_command_string(args, 0), 5, "Invalid DTM PDU type");
      return false;
      break;
  }

  txData[0] = packetType;
  txData[1] = count;

  if (railDataConfig.txMethod == PACKET_MODE) {
    RAIL_WriteTxFifo(railHandle, txData, txDataLen, true);
  }
  return true;
}

void cliSeparatorHack(sl_cli_command_arg_t *arguments);
void setBleDtmTxPayload(sl_cli_command_arg_t *args)
{
  if (setDtmTxPayloadHelper(args)) {
    args->argc = sl_cli_get_command_count(args); /* only reference cmd str */
    printTxPacket(args);
  }
}

static const sl_cli_command_info_t cli_cmd__setBleDtmTxPayload = \
  SL_CLI_COMMAND(setBleDtmTxPayload,
                 "Sets the packet payload for BLE DTM format.",
                 "Packet Type in SIG format" SL_CLI_UNIT_SEPARATOR "Packet Size in bytes" SL_CLI_UNIT_SEPARATOR,
                 {SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd___________________________ = \
  SL_CLI_COMMAND(cliSeparatorHack,
                "",
                  "",
                {SL_CLI_ARG_END, });


// Define the command table
const sl_cli_command_entry_t cli_ble_dtm_command_table[] = {
  { "___________________________", &cli_cmd___________________________, false },
  { "setBleDtmTxPayload", &cli_cmd__setBleDtmTxPayload, false },
  { NULL, NULL, false },
};

sl_cli_command_group_t cli_ble_dtm_command_group =
{
  { NULL },
  false,
  cli_ble_dtm_command_table
};

void railtest_ble_dtm_init(void)
{
  sl_cli_command_add_command_group(sl_cli_inst0_handle, &cli_ble_dtm_command_group);
}
