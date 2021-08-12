Differences Between Hadron and LSC
==================================

Difference category A - Number parsing.

<DIFFA1>

The hexadecimal prefix "0x" requires at least one digit following the prefix.

In LSC:

```
0x => 0
```

<DIFFA2>

The hexadecimal prefix "0x" must be exactly "0x".

In LSC:

```
12345xa => 10
```

