[@jprayner/piconet-nodejs](../README.md) / [Modules](../modules.md) / driver/semver

# Module: driver/semver

## Table of contents

### Type Aliases

- [Semver](driver_semver.md#semver)

### Functions

- [areVersionsCompatible](driver_semver.md#areversionscompatible)
- [parseSemver](driver_semver.md#parsesemver)

## Type Aliases

### Semver

Ƭ **Semver**: `Object`

#### Type declaration

| Name | Type |
| :------ | :------ |
| `major` | `number` |
| `minor` | `number` |
| `patch` | `number` |

#### Defined in

[driver/semver.ts:1](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/driver/semver.ts#L1)

## Functions

### areVersionsCompatible

▸ **areVersionsCompatible**(`versionA`, `versionB`): `boolean`

#### Parameters

| Name | Type |
| :------ | :------ |
| `versionA` | [`Semver`](driver_semver.md#semver) |
| `versionB` | [`Semver`](driver_semver.md#semver) |

#### Returns

`boolean`

#### Defined in

[driver/semver.ts:20](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/driver/semver.ts#L20)

___

### parseSemver

▸ **parseSemver**(`version`): [`Semver`](driver_semver.md#semver)

#### Parameters

| Name | Type |
| :------ | :------ |
| `version` | `string` |

#### Returns

[`Semver`](driver_semver.md#semver)

#### Defined in

[driver/semver.ts:7](https://github.com/jprayner/piconet/blob/81026b7/driver/nodejs/src/driver/semver.ts#L7)
