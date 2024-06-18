# RAILTEST CLI - DTM TX Extension

RAILTEST CLI extension to support ble dtm tx test payloads

## Added CLI command

This extension, when built within a RAILTEST sample application adds DTM tx payload generation support

A dedicated command will be added to the RAILTEST ones as below :

```shell
setBleDtmTxPayload  Sets the packet payload for BLE DTM format.
                    [uint8] Packet Type in SIG format
                    [uint8] Packet Size in bytes
```

Packet Payload type is coded as in [Bluetooth Spec Part F](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Core-54/out/en/low-energy-controller/direct-test-mode.html#UUID-a632201a-948b-4e33-be89-53c09429dcbe)

```c
/* LE Test packet PDU type headers - payload shown as big endian (not transmission order) */
#define   PDU_LETEST_PRBS9            0x00
#define   PDU_LETEST_0FH              0x01
#define   PDU_LETEST_55H              0x02
#define   PDU_LETEST_PRBS15           0x03
#define   PDU_LETEST_FFH              0x04
#define   PDU_LETEST_00H              0x05
#define   PDU_LETEST_F0H              0x06
#define   PDU_LETEST_AAH              0x07
```

## Usage

### CLI extension activation

The CLI extension needs to be activated by calling `railtest_ble_dtm_init();`

That API is part of `ble_dtm_cli.h`

Refer to `app.c` from this repo : 

```c
#include "app.h"
#include "ble_dtm_cli.h"

void app_init(void)
{
  railtest_ble_dtm_init();
}

void app_process_action(void)
{
}

```

### CLI extension usage

RAILtest recipe to transmit BLE DTM:

```shell
rx 0
setblemode 1
setblechannelparams 0 0x71764129 0x00555555 1
setchannel 1
setpower 100
setBleDtmTxPayload 1 37
settxdelay 1
setperipheralenable 0
tx 1000
```

Railtest - transmit DTM packets 0x0F ('11110000' transmission order), len = 37 on logical channel 0, physical channel 1, 2404 MHz, @10dBm, 1ms interval

```shell
rx 0
{{(rx)}{Rx:Disabled}{Idle:Enabled}{Time:59492422}}
> setblemode 1
{{(setblemode)}{BLE:Enabled}}
> setblechannelparams 0 0x71764129 0x00555555 1
{{(setblechannelparams)}{LogicalChannel:0}{AccessAddress:0x71764129}{CrcInit:0x00555555}{Whitening:Disabled}}
> setchannel 1
{{(setchannel)}{channel:1}}
> setpower 100
{{(setpower)}{powerLevel:79}{power:100}}
> setBleDtmTxPayload 1 37
{{(setBleDtmTxPayload)}{len:39}{payload: 0x01 0x25 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f 0x0f}}
> settxdelay 1
{{(settxdelay)}{txDelay:1}}
> setperipheralenable 0
{{(setperipheralenable)}{Peripherals:Disabled}{Notifications:Enabled}}
> tx 1000
{{(tx)}{PacketTx:Enabled}{None:Disabled}{Time:113809395}}
> {{(appMode)}{None:Enabled}{PacketTx:Disabled}{Time:115446194}}
{{(txEnd)}{txStatus:Complete}{transmitted:1000}{lastTxTime:115446083}{timePos:6}{durationUs:380}{lastTxStart:115445703}{ccaSuccess:0}{failed:0}{lastTxStatus:0x000000000}{txRemain:0}{isAck:False}}
```

Some notes:

1. The setBleDtmTxPayload takes care of calling settxlength and settxpayload to automatically set the test payload.

    The second argument (0x01 here) is the type of test packet.

    The third argument (0x25 here) is the number of test packet bytes.

2. The packet interval is 1ms here (minimum allowed by RAILtest without modification) instead of 625us standard for DTM, but it seems to work fine when a Blue Gecko NCP is receiving.

    If 625us is needed, can customize RAILtest by modifying scheduleNextTx() in mode_helpers.c to take the settxdelay argument in us and then call "settxdelay 625” on the CLI.
