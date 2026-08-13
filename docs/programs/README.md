# Sample TI-83 programs (.8xp)

Real TI-83/84 Plus `.8xp` program files for testing the **PRGM → Import
.8xp** feature. Each was built from the standard TI-BASIC token stream, so
they exercise the importer's detokeniser end to end.

To import: open **PRGM** (2ND + √( ), paste the file's full path into the
**Import .8xp** field, and press **IMPORT**. The program then appears in the
list — **RUN** it, or **EDIT** to see the decoded source.

| File | Source | Output when run |
|---|---|---|
| `HELLO.8xp` | `Disp "HELLO" : 5→A : Disp A` | `HELLO` then `5` |
| `COUNT.8xp` | `For(I,1,5) : Disp I : End` | `1 2 3 4 5` |
| `SQUARES.8xp` | `For(N,1,4) : Disp N*N : End` | `1 4 9 16` |
| `SUMTO10.8xp` | `0→S : For(I,1,10) : S+I→S : End : Disp S` | `55` |
| `BIGCHECK.8xp` | `5→X : If X>3 : Disp "BIG"` | `BIG` |

Scope: TI-83+/84+ BASIC programs. Asm programs and the TI-84+ CE colour
(`0xEF`) token page are out of scope.
