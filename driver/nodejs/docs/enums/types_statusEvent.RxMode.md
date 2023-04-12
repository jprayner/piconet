[@jprayner/piconet-nodejs](../README.md) / [Modules](../modules.md) / [types/statusEvent](../modules/types_statusEvent.md) / RxMode

# Enumeration: RxMode

[types/statusEvent](../modules/types_statusEvent.md).RxMode

Describes the current operating mode of the Piconet board.

## Table of contents

### Enumeration Members

- [LISTEN](types_statusEvent.RxMode.md#listen)
- [MONITOR](types_statusEvent.RxMode.md#monitor)
- [STOP](types_statusEvent.RxMode.md#stop)

## Enumeration Members

### LISTEN

• **LISTEN** = ``1``

The normal Econet station operating mode. The board generates events for broadcast frames
or frames targeting the configured local Econet station number. You should normally set
the station number before entering this mode.

#### Defined in

[types/statusEvent.ts:18](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/statusEvent.ts#L18)

___

### MONITOR

• **MONITOR** = ``2``

The board generates an event for every frame received, regardless of its source or
destination (promiscuous mode). Useful for capturing traffic between other stations like
the BBC NETMON utility. A code example is provided for how to build such a utility.

#### Defined in

[types/statusEvent.ts:25](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/statusEvent.ts#L25)

___

### STOP

• **STOP** = ``0``

The board starts in this mode. No events are generated in response to network traffic,
allowing the client to initialise configuration before proceeding.

#### Defined in

[types/statusEvent.ts:11](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/types/statusEvent.ts#L11)
