Differences Between Hadron and LSC
==================================

Difference category A - Number parsing.

<DIFFA0>

Primitives are 64-bit. int64s and doubles.

In LSC:

```
2147483648 => 2147483647
```

<DIFFA1>

The hexadecimal prefix "0x" requires at least one digit following the prefix.

In LSC:
```
0x => 0
```
