# Infineon Aurix Peripheral Configuration — Global Instructions

You are assisting with **Infineon Aurix MCU peripheral configuration** using Low-Level Drivers (LLD).

## MCP Tools Available

This project is connected to an **Infineon Copilot MCP server** (`infineon-copilot`) that provides:

1. **get_lld_catalog** — Browse all available LLD documentation and code examples.
2. **get_lld_content** — Retrieve specific LLD API docs, header files, or examples by name.
3. **search_user_manual** — RAG search over the Infineon Aurix reference manual for register descriptions,
   clock configuration, pin mappings, peripheral setup procedures, etc.
4. **get_peripheral_info** — Combined lookup: manual excerpts + LLD APIs for a peripheral.
5. **infineon_workspace_init** — Scaffold project into workspace (already done if you see this file).

## Workflow Rules

- **Always call `search_user_manual` first** when the user asks about a peripheral — do not
  rely on general knowledge. The ingested reference manual is the source of truth for register
  addresses, bit fields, clock requirements, and pin assignments.
- **Always call `get_lld_catalog`** to discover available LLD APIs before generating
  configuration code.
- **Use `get_lld_content`** to read the exact API signatures and parameter descriptions before
  generating a single line of code.
- **Cite manual page numbers and section names** in comments so the user can verify.
- **Generate complete, compilable configuration code** using LLD APIs — not bare register writes.
- **Include initialization order** in every configuration:
  1. Enable peripheral clock (SCU_CLK)
  2. Configure port pins (alternate functions, speed, pull resistors)
  3. Configure the peripheral (mode, speed, data format)
  4. Enable the peripheral
  5. Start transfers / conversions

## Code Style Rules

- Use `#define` for pin and port constants.
- Name init structs clearly: `port_init`, `qspi_init`, `asclin_init`, etc.
- Add comments citing register names and manual page references for every non-obvious value.
- Include a brief header comment block describing the peripheral, pins, and clock assumptions.
