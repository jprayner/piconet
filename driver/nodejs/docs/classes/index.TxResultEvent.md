[@jprayner/piconet-nodejs](../README.md) / [Modules](../modules.md) / [index](../modules/index.md) / TxResultEvent

# Class: TxResultEvent

[index](../modules/index.md).TxResultEvent

Generated in response to a `TRANSMIT` command.

## Hierarchy

- [`EconetEvent`](index.EconetEvent.md)

  ↳ **`TxResultEvent`**

## Table of contents

### Constructors

- [constructor](index.TxResultEvent.md#constructor)

### Properties

- [description](index.TxResultEvent.md#description)
- [success](index.TxResultEvent.md#success)

## Constructors

### constructor

• **new TxResultEvent**(`success`, `description`)

#### Parameters

| Name          | Type      | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| :------------ | :-------- | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `success`     | `boolean` | `true` if the `TRANSMIT` operation was successful or `false` if it failed.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| `description` | `string`  | A description of the result: `OK` — Packet was successfully sent and acknowledged `UNINITIALISED` — Firmware issue — should never happen `OVERFLOW` — Indicates that size of message exceeds TX_DATA_BUFFER_SZ `UNDERRUN` — Indicates that firmware is failing to keep up with ADLC when sending data `LINE_JAMMED` — Suggests that a station is 'flag-filling' and preventing transmission `NO_SCOUT_ACK` — Remote station failed to acknowledge scout frame (disconnected or not listening on port?) — consider retrying `NO_DATA_ACK` — Remote station failed to acknowledge data frame — consider retrying `TIMEOUT` — Other timeout condition e.g. in communication with ADLC `MISC` — Logic error e.g. in protocol decode `UNEXPECTED` — Firmware issue — should never happen |

#### Overrides

[EconetEvent](index.EconetEvent.md).[constructor](index.EconetEvent.md#constructor)

#### Defined in

[types/txResultEvent.ts:7](https://github.com/jprayner/piconet/blob/55ff188/driver/nodejs/src/types/txResultEvent.ts#L7)

## Properties

### description

• **description**: `string`

A description of the result:

`OK` — Packet was successfully sent and acknowledged

`UNINITIALISED` — Firmware issue — should never happen

`OVERFLOW` — Indicates that size of message exceeds TX_DATA_BUFFER_SZ

`UNDERRUN` — Indicates that firmware is failing to keep up with ADLC when sending data

`LINE_JAMMED` — Suggests that a station is 'flag-filling' and preventing transmission

`NO_SCOUT_ACK` — Remote station failed to acknowledge scout frame (disconnected or not listening on port?) — consider retrying

`NO_DATA_ACK` — Remote station failed to acknowledge data frame — consider retrying

`TIMEOUT` — Other timeout condition e.g. in communication with ADLC

`MISC` — Logic error e.g. in protocol decode

`UNEXPECTED` — Firmware issue — should never happen

#### Defined in

[types/txResultEvent.ts:36](https://github.com/jprayner/piconet/blob/55ff188/driver/nodejs/src/types/txResultEvent.ts#L36)

---

### success

• **success**: `boolean`

`true` if the `TRANSMIT` operation was successful or `false` if it failed.

#### Defined in

[types/txResultEvent.ts:11](https://github.com/jprayner/piconet/blob/55ff188/driver/nodejs/src/types/txResultEvent.ts#L11)
