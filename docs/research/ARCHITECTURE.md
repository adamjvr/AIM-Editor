# AIM Editor architecture

## Dependency direction

```text
UI
 |
 v
Program / Parameter Model
 |                 |
 v                 v
Editor Features   Librarian
       \           /
        v         v
         Ion Protocol
          |      |
         NRPN   SysEx
           \     /
            v   v
          MIDI Service
```

The dependency direction is deliberate: GUI code may depend on the model, but the model must not depend on GUI classes.

## Canonical data

The repository stores reverse-engineered knowledge as JSON. C++ consumes the data, but C++ is not the source of truth for the synth definition.

`data/parameters.json` begins as a visual inventory. Each protocol mapping starts in an explicit `unmapped` state and moves to `candidate`, then `verified` only when evidence supports it.

## Program representation

The semantic program model uses stable dotted parameter IDs internally, e.g. `filter1.frequency`. JSON export nests those IDs for readability:

```text
filter1.frequency -> { "filter1": { "frequency": ... } }
```

Unknown raw bytes are stored separately and survive decode/re-encode workflows.

## UI pages

- Front
- Dual 1
- Dual 2
- Randomizer
- Rear

The page names come from the reference application, but components are responsive and not tied to a fixed 1000x568 pixel canvas.

## iPadOS

The UI is landscape-first. Visual controls may remain compact while their hit areas remain touch-friendly. Tracking-generator and envelope editors should eventually support direct touch interaction.
